#include "semantic/SemanticAnalyzer.h"
#include <cmath>
#include <sstream>
#include <stdexcept>

using namespace std;

SemanticAnalyzer::SemanticAnalyzer()
    : symbolTable(), errors(), functions(), functionDeclarations(),
      currentFunctionName(), currentFunctionReturnType(ValueType::ERROR),
      loopDepth(0)
{
}

void SemanticAnalyzer::reportError(int line, const string& message)
{
    stringstream output;
    output << "Semantic error at line " << line << ": " << message;
    errors.push_back(output.str());
    errorLines.push_back(line);
}

ValueType SemanticAnalyzer::typeFromName(const string& typeName) const
{
    if (typeName == "int") return ValueType::INT;
    if (typeName == "float") return ValueType::FLOAT;
    if (typeName == "bool") return ValueType::BOOL;
    if (typeName == "string") return ValueType::STRING;
    return ValueType::ERROR;
}

ValueType SemanticAnalyzer::typeFromLiteral(TokenType literalType) const
{
    switch (literalType)
    {
        case TokenType::INTEGER_LITERAL: return ValueType::INT;
        case TokenType::FLOAT_LITERAL: return ValueType::FLOAT;
        case TokenType::STRING_LITERAL: return ValueType::STRING;
        case TokenType::TRUE:
        case TokenType::FALSE: return ValueType::BOOL;
        default: return ValueType::ERROR;
    }
}

bool SemanticAnalyzer::isNumeric(ValueType type) const
{
    return type == ValueType::INT || type == ValueType::FLOAT;
}

bool SemanticAnalyzer::isAssignable(ValueType destination, ValueType source) const
{
    if (destination == ValueType::ERROR || source == ValueType::ERROR) return true;
    if (destination == source) return true;
    return destination == ValueType::FLOAT && source == ValueType::INT;
}

ValueType SemanticAnalyzer::commonNumericType(ValueType left, ValueType right) const
{
    if (left == ValueType::FLOAT || right == ValueType::FLOAT) return ValueType::FLOAT;
    return ValueType::INT;
}

void SemanticAnalyzer::collectFunctionSignatures(const Program& program)
{
    for (const unique_ptr<Stmt>& statement : program.getStatements())
    {
        const auto* function = dynamic_cast<const FunctionStmt*>(statement.get());
        if (function == nullptr) continue;

        if (function->getName() == "str")
        {
            reportError(function->getLine(),
                        "Function 'str' is a built-in conversion and cannot be redeclared.");
            continue;
        }

        if (functions.find(function->getName()) != functions.end())
        {
            reportError(function->getLine(),
                        "Function '" + function->getName() +
                        "' is already declared.");
            continue;
        }

        FunctionSymbol symbol;
        symbol.name = function->getName();
        symbol.returnType = typeFromName(function->getReturnType());
        symbol.declarationLine = function->getLine();

        for (const FunctionParameter& parameter : function->getParameters())
        {
            symbol.parameterTypes.push_back(typeFromName(parameter.typeName));
            symbol.parameterNames.push_back(parameter.name);
        }

        functions.emplace(symbol.name, symbol);
        functionDeclarations.push_back(symbol);
    }
}

bool SemanticAnalyzer::analyze(const Program& program)
{
    symbolTable = SymbolTable();
    errors.clear();
    errorLines.clear();
    functions.clear();
    functionDeclarations.clear();
    currentFunctionName.clear();
    currentFunctionReturnType = ValueType::ERROR;
    loopDepth = 0;

    collectFunctionSignatures(program);

    // Analyze executable top-level statements first. Function signatures have
    // already been collected, so forward calls and recursion are legal.
    for (const unique_ptr<Stmt>& statement : program.getStatements())
    {
        if (dynamic_cast<const FunctionStmt*>(statement.get()) == nullptr)
            analyzeStatement(statement.get());
    }

    for (const unique_ptr<Stmt>& statement : program.getStatements())
    {
        const auto* function = dynamic_cast<const FunctionStmt*>(statement.get());
        if (function == nullptr) continue;

        auto registered = functions.find(function->getName());
        if (registered != functions.end() &&
            registered->second.declarationLine == function->getLine())
        {
            analyzeFunction(function);
        }
    }

    return errors.empty();
}

void SemanticAnalyzer::analyzeFunction(const FunctionStmt* statement)
{
    currentFunctionName = statement->getName();
    currentFunctionReturnType = typeFromName(statement->getReturnType());

    // A function body is never inside an enclosing loop of its own.
    loopDepth = 0;

    symbolTable.enterScope();

    for (const FunctionParameter& parameter : statement->getParameters())
    {
        ValueType type = typeFromName(parameter.typeName);
        if (!symbolTable.declare(parameter.name, type, parameter.line))
        {
            reportError(parameter.line,
                        "Parameter '" + parameter.name +
                        "' is duplicated in function '" + statement->getName() + "'.");
        }
    }

    for (const unique_ptr<Stmt>& child : statement->getBody()->getStatements())
        analyzeStatement(child.get());

    if (!statementAlwaysReturns(statement->getBody()))
    {
        reportError(statement->getLine(),
                    "Function '" + statement->getName() +
                    "' must return a value of type '" +
                    valueTypeToString(currentFunctionReturnType) +
                    "' on every execution path.");
    }

    symbolTable.exitScope();
    currentFunctionName.clear();
    currentFunctionReturnType = ValueType::ERROR;
}

bool SemanticAnalyzer::statementAlwaysReturns(const Stmt* statement) const
{
    if (dynamic_cast<const ReturnStmt*>(statement) != nullptr) return true;

    if (const auto* block = dynamic_cast<const BlockStmt*>(statement))
    {
        for (const unique_ptr<Stmt>& child : block->getStatements())
        {
            if (statementAlwaysReturns(child.get())) return true;
        }
        return false;
    }

    if (const auto* conditional = dynamic_cast<const IfStmt*>(statement))
    {
        return conditional->getElseBranch() != nullptr &&
               statementAlwaysReturns(conditional->getThenBranch()) &&
               statementAlwaysReturns(conditional->getElseBranch());
    }

    // A while loop is conservatively treated as not guaranteeing a return,
    // because its condition may be false on entry.
    return false;
}

void SemanticAnalyzer::analyzeStatement(const Stmt* statement)
{
    if (const auto* declaration = dynamic_cast<const VarDeclStmt*>(statement))
    {
        analyzeVariableDeclaration(declaration);
        return;
    }
    if (const auto* assignment = dynamic_cast<const AssignmentStmt*>(statement))
    {
        analyzeAssignment(assignment);
        return;
    }
    if (const auto* printStatement = dynamic_cast<const PrintStmt*>(statement))
    {
        analyzePrint(printStatement);
        return;
    }
    if (const auto* returnStatement = dynamic_cast<const ReturnStmt*>(statement))
    {
        analyzeReturn(returnStatement);
        return;
    }
    if (const auto* block = dynamic_cast<const BlockStmt*>(statement))
    {
        analyzeBlock(block);
        return;
    }
    if (const auto* ifStatement = dynamic_cast<const IfStmt*>(statement))
    {
        analyzeIf(ifStatement);
        return;
    }
    if (const auto* whileStatement = dynamic_cast<const WhileStmt*>(statement))
    {
        analyzeWhile(whileStatement);
        return;
    }
    if (const auto* forStatement = dynamic_cast<const ForStmt*>(statement))
    {
        analyzeFor(forStatement);
        return;
    }
    if (const auto* breakStatement = dynamic_cast<const BreakStmt*>(statement))
    {
        analyzeBreak(breakStatement);
        return;
    }
    if (const auto* continueStatement = dynamic_cast<const ContinueStmt*>(statement))
    {
        analyzeContinue(continueStatement);
        return;
    }
    if (dynamic_cast<const FunctionStmt*>(statement) != nullptr)
    {
        reportError(statement->getLine(), "Nested function declarations are not allowed.");
        return;
    }
    reportError(statement->getLine(), "Unknown statement type.");
}

void SemanticAnalyzer::analyzeVariableDeclaration(const VarDeclStmt* statement)
{
    ValueType declaredType = typeFromName(statement->getTypeName());
    if (declaredType == ValueType::ERROR)
    {
        reportError(statement->getLine(), "Unknown type '" + statement->getTypeName() + "'.");
        return;
    }

    bool duplicate = symbolTable.isDeclaredInCurrentScope(statement->getName());
    if (statement->getInitializer() != nullptr)
    {
        ValueType initializerType = analyzeExpression(statement->getInitializer());
        if (!isAssignable(declaredType, initializerType))
        {
            reportError(statement->getLine(),
                        "Cannot initialize variable '" + statement->getName() +
                        "' of type '" + valueTypeToString(declaredType) +
                        "' with a value of type '" + valueTypeToString(initializerType) + "'.");
        }
    }

    if (duplicate)
    {
        reportError(statement->getLine(),
                    "Variable '" + statement->getName() +
                    "' is already declared in this scope.");
        return;
    }

    symbolTable.declare(statement->getName(), declaredType, statement->getLine());
}

void SemanticAnalyzer::analyzeAssignment(const AssignmentStmt* statement)
{
    const Symbol* symbol = symbolTable.lookup(statement->getName());
    ValueType valueType = analyzeExpression(statement->getValue());

    if (symbol == nullptr)
    {
        reportError(statement->getLine(),
                    "Variable '" + statement->getName() + "' has not been declared.");
        return;
    }

    if (!currentFunctionName.empty() && symbol->scopeDepth == 0)
    {
        reportError(statement->getLine(),
                    "Function '" + currentFunctionName +
                    "' cannot assign global variable '" + statement->getName() +
                    "'; pass data through parameters and return values instead.");
        return;
    }

    if (!isAssignable(symbol->type, valueType))
    {
        reportError(statement->getLine(),
                    "Cannot assign a value of type '" + valueTypeToString(valueType) +
                    "' to variable '" + statement->getName() + "' of type '" +
                    valueTypeToString(symbol->type) + "'.");
    }
}

void SemanticAnalyzer::analyzePrint(const PrintStmt* statement)
{
    analyzeExpression(statement->getExpression());
}

void SemanticAnalyzer::analyzeReturn(const ReturnStmt* statement)
{
    ValueType valueType = analyzeExpression(statement->getExpression());
    if (currentFunctionName.empty())
    {
        reportError(statement->getLine(), "A return statement is only valid inside a function.");
        return;
    }

    if (!isAssignable(currentFunctionReturnType, valueType))
    {
        reportError(statement->getLine(),
                    "Function '" + currentFunctionName + "' returns type '" +
                    valueTypeToString(currentFunctionReturnType) +
                    "', but this return expression has type '" +
                    valueTypeToString(valueType) + "'.");
    }
}

void SemanticAnalyzer::analyzeBlock(const BlockStmt* statement)
{
    symbolTable.enterScope();
    for (const unique_ptr<Stmt>& child : statement->getStatements())
        analyzeStatement(child.get());
    symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeIf(const IfStmt* statement)
{
    ValueType conditionType = analyzeExpression(statement->getCondition());
    if (conditionType != ValueType::BOOL && conditionType != ValueType::ERROR)
    {
        reportError(statement->getCondition()->getLine(),
                    "The condition of an if statement must have type 'bool', not '" +
                    valueTypeToString(conditionType) + "'.");
    }
    analyzeStatement(statement->getThenBranch());
    if (statement->getElseBranch() != nullptr)
        analyzeStatement(statement->getElseBranch());
}

void SemanticAnalyzer::analyzeWhile(const WhileStmt* statement)
{
    ValueType conditionType = analyzeExpression(statement->getCondition());
    if (conditionType != ValueType::BOOL && conditionType != ValueType::ERROR)
    {
        reportError(statement->getCondition()->getLine(),
                    "The condition of a while statement must have type 'bool', not '" +
                    valueTypeToString(conditionType) + "'.");
    }
    loopDepth++;
    analyzeStatement(statement->getBody());
    loopDepth--;
}

void SemanticAnalyzer::analyzeFor(const ForStmt* statement)
{
    // The header gets its own scope so that a variable declared in the
    // initializer is visible to the condition, increment, and body, but not
    // after the loop.
    symbolTable.enterScope();

    analyzeStatement(statement->getInitializer());

    ValueType conditionType = analyzeExpression(statement->getCondition());
    if (conditionType != ValueType::BOOL && conditionType != ValueType::ERROR)
    {
        reportError(statement->getCondition()->getLine(),
                    "The condition of a for statement must have type 'bool', not '" +
                    valueTypeToString(conditionType) + "'.");
    }

    analyzeStatement(statement->getIncrement());

    loopDepth++;
    analyzeStatement(statement->getBody());
    loopDepth--;

    symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeBreak(const BreakStmt* statement)
{
    if (loopDepth == 0)
    {
        reportError(statement->getLine(),
                    "A break statement is only valid inside a while or for loop.");
    }
}

void SemanticAnalyzer::analyzeContinue(const ContinueStmt* statement)
{
    if (loopDepth == 0)
    {
        reportError(statement->getLine(),
                    "A continue statement is only valid inside a while or for loop.");
    }
}

ValueType SemanticAnalyzer::analyzeExpression(const Expr* expression)
{
    if (const auto* literal = dynamic_cast<const LiteralExpr*>(expression)) return analyzeLiteral(literal);
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expression)) return analyzeVariable(variable);
    if (const auto* call = dynamic_cast<const CallExpr*>(expression)) return analyzeCall(call);
    if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression)) return analyzeUnary(unary);
    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) return analyzeBinary(binary);
    if (const auto* grouping = dynamic_cast<const GroupingExpr*>(expression)) return analyzeGrouping(grouping);
    reportError(expression->getLine(), "Unknown expression type.");
    return ValueType::ERROR;
}

ValueType SemanticAnalyzer::analyzeLiteral(const LiteralExpr* expression)
{
    ValueType type = typeFromLiteral(expression->getLiteralType());
    if (type == ValueType::INT)
    {
        try
        {
            size_t consumed = 0;
            stoll(expression->getValue(), &consumed);
            if (consumed != expression->getValue().size())
            {
                reportError(expression->getLine(), "Invalid integer literal '" + expression->getValue() + "'.");
                return ValueType::ERROR;
            }
        }
        catch (const invalid_argument&)
        {
            reportError(expression->getLine(), "Invalid integer literal '" + expression->getValue() + "'.");
            return ValueType::ERROR;
        }
        catch (const out_of_range&)
        {
            reportError(expression->getLine(),
                        "Integer literal is outside the supported 64-bit signed range: '" +
                        expression->getValue() + "'.");
            return ValueType::ERROR;
        }
    }
    else if (type == ValueType::FLOAT)
    {
        try
        {
            size_t consumed = 0;
            double value = stod(expression->getValue(), &consumed);
            if (consumed != expression->getValue().size() || !isfinite(value))
            {
                reportError(expression->getLine(),
                            "Float literal is outside the supported finite range: '" +
                            expression->getValue() + "'.");
                return ValueType::ERROR;
            }
        }
        catch (const invalid_argument&)
        {
            reportError(expression->getLine(), "Invalid float literal '" + expression->getValue() + "'.");
            return ValueType::ERROR;
        }
        catch (const out_of_range&)
        {
            reportError(expression->getLine(),
                        "Float literal is outside the supported finite range: '" +
                        expression->getValue() + "'.");
            return ValueType::ERROR;
        }
    }
    return type;
}

ValueType SemanticAnalyzer::analyzeVariable(const VariableExpr* expression)
{
    const Symbol* symbol = symbolTable.lookup(expression->getName());
    if (symbol == nullptr)
    {
        reportError(expression->getLine(),
                    "Variable '" + expression->getName() + "' has not been declared.");
        return ValueType::ERROR;
    }

    if (!currentFunctionName.empty() && symbol->scopeDepth == 0)
    {
        reportError(expression->getLine(),
                    "Function '" + currentFunctionName +
                    "' cannot read global variable '" + expression->getName() +
                    "'; pass it as a parameter instead.");
        return ValueType::ERROR;
    }

    return symbol->type;
}

ValueType SemanticAnalyzer::analyzeCall(const CallExpr* expression)
{
    // str(x) is a built-in conversion rather than a declared function. It
    // accepts any single non-error value and always yields a string, which
    // lets "Age: " + str(25) work without making '+' silently coerce.
    if (expression->getCallee() == "str")
    {
        const vector<unique_ptr<Expr>>& arguments = expression->getArguments();

        for (const unique_ptr<Expr>& argument : arguments)
        {
            analyzeExpression(argument.get());
        }

        if (arguments.size() != 1)
        {
            reportError(expression->getLine(),
                        "Built-in function 'str' expects 1 argument(s), but received " +
                        to_string(arguments.size()) + ".");
        }

        // Always a string, even after an error, so one bad call does not
        // cascade into further type errors further up the expression.
        return ValueType::STRING;
    }

    auto found = functions.find(expression->getCallee());
    if (found == functions.end())
    {
        for (const unique_ptr<Expr>& argument : expression->getArguments())
            analyzeExpression(argument.get());
        reportError(expression->getLine(),
                    "Function '" + expression->getCallee() + "' has not been declared.");
        return ValueType::ERROR;
    }

    const FunctionSymbol& function = found->second;
    const vector<unique_ptr<Expr>>& arguments = expression->getArguments();
    if (arguments.size() != function.parameterTypes.size())
    {
        reportError(expression->getLine(),
                    "Function '" + expression->getCallee() + "' expects " +
                    to_string(function.parameterTypes.size()) + " argument(s), but received " +
                    to_string(arguments.size()) + ".");
    }

    size_t shared = arguments.size() < function.parameterTypes.size()
                        ? arguments.size() : function.parameterTypes.size();
    for (size_t index = 0; index < arguments.size(); index++)
    {
        ValueType argumentType = analyzeExpression(arguments[index].get());
        if (index < shared && !isAssignable(function.parameterTypes[index], argumentType))
        {
            reportError(arguments[index]->getLine(),
                        "Argument " + to_string(index + 1) + " of function '" +
                        expression->getCallee() + "' expects type '" +
                        valueTypeToString(function.parameterTypes[index]) +
                        "', but received '" + valueTypeToString(argumentType) + "'.");
        }
    }

    return function.returnType;
}

ValueType SemanticAnalyzer::analyzeUnary(const UnaryExpr* expression)
{
    ValueType operandType = analyzeExpression(expression->getRight());
    TokenType operatorType = expression->getOperator().type;
    if (operandType == ValueType::ERROR) return ValueType::ERROR;

    if (operatorType == TokenType::NOT)
    {
        if (operandType != ValueType::BOOL)
        {
            reportError(expression->getLine(),
                        "Operator '!' requires a bool operand, but received '" +
                        valueTypeToString(operandType) + "'.");
            return ValueType::ERROR;
        }
        return ValueType::BOOL;
    }

    if (operatorType == TokenType::PLUS || operatorType == TokenType::MINUS)
    {
        if (!isNumeric(operandType))
        {
            reportError(expression->getLine(),
                        "Unary operator '" + expression->getOperator().lexeme +
                        "' requires a numeric operand, but received '" +
                        valueTypeToString(operandType) + "'.");
            return ValueType::ERROR;
        }
        return operandType;
    }

    reportError(expression->getLine(),
                "Unsupported unary operator '" + expression->getOperator().lexeme + "'.");
    return ValueType::ERROR;
}

ValueType SemanticAnalyzer::analyzeBinary(const BinaryExpr* expression)
{
    ValueType leftType = analyzeExpression(expression->getLeft());
    ValueType rightType = analyzeExpression(expression->getRight());
    TokenType operatorType = expression->getOperator().type;
    const string& operatorText = expression->getOperator().lexeme;
    if (leftType == ValueType::ERROR || rightType == ValueType::ERROR) return ValueType::ERROR;

    if (operatorType == TokenType::PLUS)
    {
        if (isNumeric(leftType) && isNumeric(rightType)) return commonNumericType(leftType, rightType);
        if (leftType == ValueType::STRING && rightType == ValueType::STRING) return ValueType::STRING;
        reportError(expression->getLine(),
                    "Operator '+' requires two numeric operands or two string operands, but received '" +
                    valueTypeToString(leftType) + "' and '" + valueTypeToString(rightType) + "'.");
        return ValueType::ERROR;
    }

    if (operatorType == TokenType::MINUS || operatorType == TokenType::MULTIPLY ||
        operatorType == TokenType::DIVIDE)
    {
        if (!isNumeric(leftType) || !isNumeric(rightType))
        {
            reportError(expression->getLine(),
                        "Operator '" + operatorText + "' requires numeric operands, but received '" +
                        valueTypeToString(leftType) + "' and '" + valueTypeToString(rightType) + "'.");
            return ValueType::ERROR;
        }
        return commonNumericType(leftType, rightType);
    }

    if (operatorType == TokenType::MODULO)
    {
        if (leftType != ValueType::INT || rightType != ValueType::INT)
        {
            reportError(expression->getLine(),
                        "Operator '%' requires int operands, but received '" +
                        valueTypeToString(leftType) + "' and '" + valueTypeToString(rightType) + "'.");
            return ValueType::ERROR;
        }
        return ValueType::INT;
    }

    if (operatorType == TokenType::GREATER || operatorType == TokenType::GREATER_EQUAL ||
        operatorType == TokenType::LESS || operatorType == TokenType::LESS_EQUAL)
    {
        if (!isNumeric(leftType) || !isNumeric(rightType))
        {
            reportError(expression->getLine(),
                        "Operator '" + operatorText + "' requires numeric operands, but received '" +
                        valueTypeToString(leftType) + "' and '" + valueTypeToString(rightType) + "'.");
            return ValueType::ERROR;
        }
        return ValueType::BOOL;
    }

    if (operatorType == TokenType::EQUAL || operatorType == TokenType::NOT_EQUAL)
    {
        bool compatible = leftType == rightType || (isNumeric(leftType) && isNumeric(rightType));
        if (!compatible)
        {
            reportError(expression->getLine(),
                        "Operator '" + operatorText + "' cannot compare values of type '" +
                        valueTypeToString(leftType) + "' and '" + valueTypeToString(rightType) + "'.");
            return ValueType::ERROR;
        }
        return ValueType::BOOL;
    }

    if (operatorType == TokenType::AND || operatorType == TokenType::OR)
    {
        if (leftType != ValueType::BOOL || rightType != ValueType::BOOL)
        {
            reportError(expression->getLine(),
                        "Operator '" + operatorText + "' requires bool operands, but received '" +
                        valueTypeToString(leftType) + "' and '" + valueTypeToString(rightType) + "'.");
            return ValueType::ERROR;
        }
        return ValueType::BOOL;
    }

    reportError(expression->getLine(), "Unsupported binary operator '" + operatorText + "'.");
    return ValueType::ERROR;
}

ValueType SemanticAnalyzer::analyzeGrouping(const GroupingExpr* expression)
{
    return analyzeExpression(expression->getExpression());
}

const vector<string>& SemanticAnalyzer::getErrors() const { return errors; }
const vector<int>& SemanticAnalyzer::getErrorLines() const { return errorLines; }
const SymbolTable& SemanticAnalyzer::getSymbolTable() const { return symbolTable; }
const vector<FunctionSymbol>& SemanticAnalyzer::getFunctionDeclarations() const
{
    return functionDeclarations;
}
