#include "AST.h"
#include <iostream>
#include <utility>

using namespace std;

namespace
{
    void indent(int indentLevel)
    {
        for (int i = 0; i < indentLevel; i++)
        {
            cout << "  ";
        }
    }
}

Expr::Expr(int sourceLine) : line(sourceLine) {}
int Expr::getLine() const { return line; }

LiteralExpr::LiteralExpr(const string& literalValue, TokenType type, int sourceLine)
    : Expr(sourceLine), value(literalValue), literalType(type) {}
const string& LiteralExpr::getValue() const { return value; }
TokenType LiteralExpr::getLiteralType() const { return literalType; }
void LiteralExpr::print(int indentLevel) const
{
    indent(indentLevel);
    if (literalType == TokenType::STRING_LITERAL)
        cout << "Literal string \"" << value << "\"" << endl;
    else
        cout << "Literal " << value << endl;
}

VariableExpr::VariableExpr(const string& variableName, int sourceLine)
    : Expr(sourceLine), name(variableName) {}
const string& VariableExpr::getName() const { return name; }
void VariableExpr::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Variable " << name << endl;
}

CallExpr::CallExpr(const string& functionName,
                   vector<unique_ptr<Expr>> callArguments,
                   int sourceLine)
    : Expr(sourceLine), callee(functionName), arguments(move(callArguments)) {}
const string& CallExpr::getCallee() const { return callee; }
const vector<unique_ptr<Expr>>& CallExpr::getArguments() const { return arguments; }
void CallExpr::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Call " << callee << endl;
    for (const unique_ptr<Expr>& argument : arguments)
        argument->print(indentLevel + 1);
}

UnaryExpr::UnaryExpr(const Token& operatorToken, unique_ptr<Expr> rightExpression)
    : Expr(operatorToken.line), op(operatorToken), right(move(rightExpression)) {}
const Token& UnaryExpr::getOperator() const { return op; }
const Expr* UnaryExpr::getRight() const { return right.get(); }
void UnaryExpr::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Unary " << op.lexeme << endl;
    right->print(indentLevel + 1);
}

BinaryExpr::BinaryExpr(unique_ptr<Expr> leftExpression,
                       const Token& operatorToken,
                       unique_ptr<Expr> rightExpression)
    : Expr(operatorToken.line), left(move(leftExpression)), op(operatorToken),
      right(move(rightExpression)) {}
const Expr* BinaryExpr::getLeft() const { return left.get(); }
const Token& BinaryExpr::getOperator() const { return op; }
const Expr* BinaryExpr::getRight() const { return right.get(); }
void BinaryExpr::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Binary " << op.lexeme << endl;
    left->print(indentLevel + 1);
    right->print(indentLevel + 1);
}

GroupingExpr::GroupingExpr(unique_ptr<Expr> innerExpression, int sourceLine)
    : Expr(sourceLine), expression(move(innerExpression)) {}
const Expr* GroupingExpr::getExpression() const { return expression.get(); }
void GroupingExpr::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Grouping" << endl;
    expression->print(indentLevel + 1);
}

Stmt::Stmt(int sourceLine) : line(sourceLine) {}
int Stmt::getLine() const { return line; }

VarDeclStmt::VarDeclStmt(const string& declaredType,
                         const string& variableName,
                         unique_ptr<Expr> initialValue,
                         int sourceLine)
    : Stmt(sourceLine), typeName(declaredType), name(variableName),
      initializer(move(initialValue)) {}
const string& VarDeclStmt::getTypeName() const { return typeName; }
const string& VarDeclStmt::getName() const { return name; }
const Expr* VarDeclStmt::getInitializer() const { return initializer.get(); }
void VarDeclStmt::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "VariableDeclaration " << typeName << " " << name << endl;
    if (initializer)
    {
        indent(indentLevel + 1);
        cout << "Initializer" << endl;
        initializer->print(indentLevel + 2);
    }
}

AssignmentStmt::AssignmentStmt(const string& variableName,
                               unique_ptr<Expr> assignedValue,
                               int sourceLine)
    : Stmt(sourceLine), name(variableName), value(move(assignedValue)) {}
const string& AssignmentStmt::getName() const { return name; }
const Expr* AssignmentStmt::getValue() const { return value.get(); }
void AssignmentStmt::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Assignment " << name << endl;
    value->print(indentLevel + 1);
}

PrintStmt::PrintStmt(unique_ptr<Expr> value, int sourceLine)
    : Stmt(sourceLine), expression(move(value)) {}
const Expr* PrintStmt::getExpression() const { return expression.get(); }
void PrintStmt::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Print" << endl;
    expression->print(indentLevel + 1);
}

ReturnStmt::ReturnStmt(unique_ptr<Expr> value, int sourceLine)
    : Stmt(sourceLine), expression(move(value)) {}
const Expr* ReturnStmt::getExpression() const { return expression.get(); }
void ReturnStmt::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Return" << endl;
    expression->print(indentLevel + 1);
}

BlockStmt::BlockStmt(vector<unique_ptr<Stmt>> blockStatements, int sourceLine)
    : Stmt(sourceLine), statements(move(blockStatements)) {}
const vector<unique_ptr<Stmt>>& BlockStmt::getStatements() const { return statements; }
void BlockStmt::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Block" << endl;
    for (const unique_ptr<Stmt>& statement : statements)
        statement->print(indentLevel + 1);
}

FunctionStmt::FunctionStmt(const string& declaredReturnType,
                           const string& functionName,
                           vector<FunctionParameter> functionParameters,
                           unique_ptr<BlockStmt> functionBody,
                           int sourceLine)
    : Stmt(sourceLine), returnType(declaredReturnType), name(functionName),
      parameters(move(functionParameters)), body(move(functionBody)) {}
const string& FunctionStmt::getReturnType() const { return returnType; }
const string& FunctionStmt::getName() const { return name; }
const vector<FunctionParameter>& FunctionStmt::getParameters() const { return parameters; }
const BlockStmt* FunctionStmt::getBody() const { return body.get(); }
void FunctionStmt::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "Function " << returnType << " " << name << endl;
    if (!parameters.empty())
    {
        indent(indentLevel + 1);
        cout << "Parameters" << endl;
        for (const FunctionParameter& parameter : parameters)
        {
            indent(indentLevel + 2);
            cout << parameter.typeName << " " << parameter.name << endl;
        }
    }
    indent(indentLevel + 1);
    cout << "Body" << endl;
    body->print(indentLevel + 2);
}

IfStmt::IfStmt(unique_ptr<Expr> conditionExpression,
               unique_ptr<Stmt> thenStatement,
               unique_ptr<Stmt> elseStatement,
               int sourceLine)
    : Stmt(sourceLine), condition(move(conditionExpression)),
      thenBranch(move(thenStatement)), elseBranch(move(elseStatement)) {}
const Expr* IfStmt::getCondition() const { return condition.get(); }
const Stmt* IfStmt::getThenBranch() const { return thenBranch.get(); }
const Stmt* IfStmt::getElseBranch() const { return elseBranch.get(); }
void IfStmt::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "If" << endl;
    indent(indentLevel + 1);
    cout << "Condition" << endl;
    condition->print(indentLevel + 2);
    indent(indentLevel + 1);
    cout << "Then" << endl;
    thenBranch->print(indentLevel + 2);
    if (elseBranch)
    {
        indent(indentLevel + 1);
        cout << "Else" << endl;
        elseBranch->print(indentLevel + 2);
    }
}

WhileStmt::WhileStmt(unique_ptr<Expr> conditionExpression,
                     unique_ptr<Stmt> bodyStatement,
                     int sourceLine)
    : Stmt(sourceLine), condition(move(conditionExpression)), body(move(bodyStatement)) {}
const Expr* WhileStmt::getCondition() const { return condition.get(); }
const Stmt* WhileStmt::getBody() const { return body.get(); }
void WhileStmt::print(int indentLevel) const
{
    indent(indentLevel);
    cout << "While" << endl;
    indent(indentLevel + 1);
    cout << "Condition" << endl;
    condition->print(indentLevel + 2);
    indent(indentLevel + 1);
    cout << "Body" << endl;
    body->print(indentLevel + 2);
}

Program::Program(vector<unique_ptr<Stmt>> programStatements)
    : statements(move(programStatements)) {}
const vector<unique_ptr<Stmt>>& Program::getStatements() const { return statements; }
void Program::print() const
{
    cout << "Program" << endl;
    for (const unique_ptr<Stmt>& statement : statements)
        statement->print(1);
}
