#include "IRGenerator.h"
#include <stdexcept>
#include <utility>

using namespace std;

IRGenerator::IRGenerator()
    : code(), temporaryCounter(0), labelCounter(0), variableCounters(),
      scopes(), functionReturnTypes(), currentFunctionReturnType()
{
}

string IRGenerator::newTemporary()
{
    temporaryCounter++;
    return "%t" + to_string(temporaryCounter);
}

string IRGenerator::newLabel()
{
    labelCounter++;
    return "L" + to_string(labelCounter);
}

void IRGenerator::enterScope()
{
    scopes.emplace_back();
}

void IRGenerator::exitScope()
{
    if (scopes.empty())
        throw runtime_error("IR generation error: attempted to exit a missing scope.");
    scopes.pop_back();
}

string IRGenerator::declareVariable(const string& sourceName)
{
    if (scopes.empty())
        throw runtime_error("IR generation error: no active scope.");

    int declarationNumber = variableCounters[sourceName];
    string irName = sourceName;
    if (declarationNumber > 0)
        irName += "$" + to_string(declarationNumber);

    variableCounters[sourceName] = declarationNumber + 1;
    scopes.back()[sourceName] = irName;
    return irName;
}

string IRGenerator::resolveVariable(const string& sourceName) const
{
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope)
    {
        auto found = scope->find(sourceName);
        if (found != scope->end()) return found->second;
    }
    throw runtime_error("IR generation error: unresolved variable '" + sourceName + "'.");
}

string IRGenerator::quoteString(const string& value) const
{
    string result = "\"";
    for (char character : value)
    {
        switch (character)
        {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default: result += character; break;
        }
    }
    result += "\"";
    return result;
}

string IRGenerator::generateLiteral(const LiteralExpr* expression) const
{
    if (expression->getLiteralType() == TokenType::STRING_LITERAL)
        return quoteString(expression->getValue());
    return expression->getValue();
}

string IRGenerator::generateExpression(const Expr* expression)
{
    if (const auto* literal = dynamic_cast<const LiteralExpr*>(expression))
        return generateLiteral(literal);

    if (const auto* variable = dynamic_cast<const VariableExpr*>(expression))
        return resolveVariable(variable->getName());

    if (const auto* call = dynamic_cast<const CallExpr*>(expression))
    {
        for (const unique_ptr<Expr>& argument : call->getArguments())
        {
            string value = generateExpression(argument.get());
            code.emit({IROpCode::ARG, "", value, "", "", "", "", argument->getLine()});
        }

        auto found = functionReturnTypes.find(call->getCallee());
        if (found == functionReturnTypes.end())
            throw runtime_error("IR generation error: unknown function '" + call->getCallee() + "'.");

        string temporary = newTemporary();
        code.emit({IROpCode::CALL,
                   temporary,
                   "",
                   "",
                   to_string(call->getArguments().size()),
                   found->second,
                   call->getCallee(),
                   call->getLine()});
        return temporary;
    }

    if (const auto* grouping = dynamic_cast<const GroupingExpr*>(expression))
        return generateExpression(grouping->getExpression());

    if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression))
    {
        string operand = generateExpression(unary->getRight());
        string temporary = newTemporary();
        code.emit({IROpCode::UNARY, temporary, operand, "",
                   unary->getOperator().lexeme, "", "", expression->getLine()});
        return temporary;
    }

    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression))
    {
        string left = generateExpression(binary->getLeft());
        string right = generateExpression(binary->getRight());
        string temporary = newTemporary();
        code.emit({IROpCode::BINARY, temporary, left, right,
                   binary->getOperator().lexeme, "", "", expression->getLine()});
        return temporary;
    }

    throw runtime_error("IR generation error: unknown expression type.");
}

void IRGenerator::generateVariableDeclaration(const VarDeclStmt* statement)
{
    string initializer;
    if (statement->getInitializer() != nullptr)
        initializer = generateExpression(statement->getInitializer());

    string variable = declareVariable(statement->getName());
    code.emit({IROpCode::DECLARE, variable, "", "", "",
               statement->getTypeName(), "", statement->getLine()});

    if (statement->getInitializer() != nullptr)
    {
        code.emit({IROpCode::ASSIGN, variable, initializer, "", "", "", "",
                   statement->getLine()});
        return;
    }

    string defaultValue;
    if (statement->getTypeName() == "int") defaultValue = "0";
    else if (statement->getTypeName() == "float") defaultValue = "0.0";
    else if (statement->getTypeName() == "bool") defaultValue = "false";
    else if (statement->getTypeName() == "string") defaultValue = "\"\"";
    else throw runtime_error("IR generation error: unsupported declaration type '" +
                             statement->getTypeName() + "'.");

    code.emit({IROpCode::ASSIGN, variable, defaultValue, "", "", "", "",
               statement->getLine()});
}

void IRGenerator::generateAssignment(const AssignmentStmt* statement)
{
    string value = generateExpression(statement->getValue());
    string variable = resolveVariable(statement->getName());
    code.emit({IROpCode::ASSIGN, variable, value, "", "", "", "", statement->getLine()});
}

void IRGenerator::generatePrint(const PrintStmt* statement)
{
    string value = generateExpression(statement->getExpression());
    code.emit({IROpCode::PRINT, "", value, "", "", "", "", statement->getLine()});
}

void IRGenerator::generateReturn(const ReturnStmt* statement)
{
    if (currentFunctionReturnType.empty())
        throw runtime_error("IR generation error: return outside function.");

    string value = generateExpression(statement->getExpression());
    code.emit({IROpCode::RETURN, "", value, "", "",
               currentFunctionReturnType, "", statement->getLine()});
}

void IRGenerator::generateBlock(const BlockStmt* statement)
{
    enterScope();
    for (const unique_ptr<Stmt>& child : statement->getStatements())
        generateStatement(child.get());
    exitScope();
}

void IRGenerator::generateIf(const IfStmt* statement)
{
    string condition = generateExpression(statement->getCondition());
    string falseLabel = newLabel();
    code.emit({IROpCode::IF_FALSE_GOTO, "", condition, "", "", "",
               falseLabel, statement->getLine()});

    generateStatement(statement->getThenBranch());
    if (statement->getElseBranch() != nullptr)
    {
        string endLabel = newLabel();
        code.emit({IROpCode::GOTO, "", "", "", "", "", endLabel, statement->getLine()});
        code.emit({IROpCode::LABEL, "", "", "", "", "", falseLabel, statement->getLine()});
        generateStatement(statement->getElseBranch());
        code.emit({IROpCode::LABEL, "", "", "", "", "", endLabel, statement->getLine()});
    }
    else
    {
        code.emit({IROpCode::LABEL, "", "", "", "", "", falseLabel, statement->getLine()});
    }
}

void IRGenerator::generateWhile(const WhileStmt* statement)
{
    string startLabel = newLabel();
    string endLabel = newLabel();
    code.emit({IROpCode::LABEL, "", "", "", "", "", startLabel, statement->getLine()});
    string condition = generateExpression(statement->getCondition());
    code.emit({IROpCode::IF_FALSE_GOTO, "", condition, "", "", "", endLabel, statement->getLine()});
    generateStatement(statement->getBody());
    code.emit({IROpCode::GOTO, "", "", "", "", "", startLabel, statement->getLine()});
    code.emit({IROpCode::LABEL, "", "", "", "", "", endLabel, statement->getLine()});
}

void IRGenerator::generateFunction(const FunctionStmt* statement)
{
    code.emit({IROpCode::FUNCTION_BEGIN, "", "", "", "",
               statement->getReturnType(), statement->getName(), statement->getLine()});

    enterScope();
    currentFunctionReturnType = statement->getReturnType();

    for (const FunctionParameter& parameter : statement->getParameters())
    {
        string name = declareVariable(parameter.name);
        code.emit({IROpCode::PARAM, name, "", "", "",
                   parameter.typeName, "", parameter.line});
    }

    for (const unique_ptr<Stmt>& child : statement->getBody()->getStatements())
        generateStatement(child.get());

    currentFunctionReturnType.clear();
    exitScope();
    code.emit({IROpCode::FUNCTION_END, "", "", "", "",
               statement->getReturnType(), statement->getName(), statement->getLine()});
}

void IRGenerator::generateStatement(const Stmt* statement)
{
    if (const auto* declaration = dynamic_cast<const VarDeclStmt*>(statement))
        return generateVariableDeclaration(declaration);
    if (const auto* assignment = dynamic_cast<const AssignmentStmt*>(statement))
        return generateAssignment(assignment);
    if (const auto* printStatement = dynamic_cast<const PrintStmt*>(statement))
        return generatePrint(printStatement);
    if (const auto* returnStatement = dynamic_cast<const ReturnStmt*>(statement))
        return generateReturn(returnStatement);
    if (const auto* block = dynamic_cast<const BlockStmt*>(statement))
        return generateBlock(block);
    if (const auto* ifStatement = dynamic_cast<const IfStmt*>(statement))
        return generateIf(ifStatement);
    if (const auto* whileStatement = dynamic_cast<const WhileStmt*>(statement))
        return generateWhile(whileStatement);
    if (dynamic_cast<const FunctionStmt*>(statement) != nullptr)
        throw runtime_error("IR generation error: nested function declaration.");
    throw runtime_error("IR generation error: unknown statement type.");
}

IntermediateCode IRGenerator::generate(const Program& program)
{
    code = IntermediateCode();
    temporaryCounter = 0;
    labelCounter = 0;
    variableCounters.clear();
    scopes.clear();
    functionReturnTypes.clear();
    currentFunctionReturnType.clear();

    for (const unique_ptr<Stmt>& statement : program.getStatements())
    {
        if (const auto* function = dynamic_cast<const FunctionStmt*>(statement.get()))
            functionReturnTypes[function->getName()] = function->getReturnType();
    }

    enterScope();

    // Main/top-level code is emitted first, which lets the bytecode generator
    // place a HALT before function bodies while still patching calls forward.
    for (const unique_ptr<Stmt>& statement : program.getStatements())
    {
        if (dynamic_cast<const FunctionStmt*>(statement.get()) == nullptr)
            generateStatement(statement.get());
    }

    for (const unique_ptr<Stmt>& statement : program.getStatements())
    {
        if (const auto* function = dynamic_cast<const FunctionStmt*>(statement.get()))
            generateFunction(function);
    }

    exitScope();
    return code;
}
