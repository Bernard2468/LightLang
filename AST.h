#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>
#include "Token.h"

using namespace std;

class Expr
{
private:
    int line;

public:
    explicit Expr(int sourceLine);
    virtual ~Expr() = default;

    int getLine() const;
    virtual void print(int indentLevel) const = 0;
};

class LiteralExpr : public Expr
{
private:
    string value;
    TokenType literalType;

public:
    LiteralExpr(const string& literalValue, TokenType type, int sourceLine);

    const string& getValue() const;
    TokenType getLiteralType() const;
    void print(int indentLevel) const override;
};

class VariableExpr : public Expr
{
private:
    string name;

public:
    VariableExpr(const string& variableName, int sourceLine);

    const string& getName() const;
    void print(int indentLevel) const override;
};

class CallExpr : public Expr
{
private:
    string callee;
    vector<unique_ptr<Expr>> arguments;

public:
    CallExpr(const string& functionName,
             vector<unique_ptr<Expr>> callArguments,
             int sourceLine);

    const string& getCallee() const;
    const vector<unique_ptr<Expr>>& getArguments() const;
    void print(int indentLevel) const override;
};

class UnaryExpr : public Expr
{
private:
    Token op;
    unique_ptr<Expr> right;

public:
    UnaryExpr(const Token& operatorToken, unique_ptr<Expr> rightExpression);

    const Token& getOperator() const;
    const Expr* getRight() const;
    void print(int indentLevel) const override;
};

class BinaryExpr : public Expr
{
private:
    unique_ptr<Expr> left;
    Token op;
    unique_ptr<Expr> right;

public:
    BinaryExpr(unique_ptr<Expr> leftExpression,
               const Token& operatorToken,
               unique_ptr<Expr> rightExpression);

    const Expr* getLeft() const;
    const Token& getOperator() const;
    const Expr* getRight() const;
    void print(int indentLevel) const override;
};

class GroupingExpr : public Expr
{
private:
    unique_ptr<Expr> expression;

public:
    GroupingExpr(unique_ptr<Expr> innerExpression, int sourceLine);

    const Expr* getExpression() const;
    void print(int indentLevel) const override;
};

class Stmt
{
private:
    int line;

public:
    explicit Stmt(int sourceLine);
    virtual ~Stmt() = default;

    int getLine() const;
    virtual void print(int indentLevel) const = 0;
};

class VarDeclStmt : public Stmt
{
private:
    string typeName;
    string name;
    unique_ptr<Expr> initializer;

public:
    VarDeclStmt(const string& declaredType,
                const string& variableName,
                unique_ptr<Expr> initialValue,
                int sourceLine);

    const string& getTypeName() const;
    const string& getName() const;
    const Expr* getInitializer() const;
    void print(int indentLevel) const override;
};

class AssignmentStmt : public Stmt
{
private:
    string name;
    unique_ptr<Expr> value;

public:
    AssignmentStmt(const string& variableName,
                   unique_ptr<Expr> assignedValue,
                   int sourceLine);

    const string& getName() const;
    const Expr* getValue() const;
    void print(int indentLevel) const override;
};

class PrintStmt : public Stmt
{
private:
    unique_ptr<Expr> expression;

public:
    PrintStmt(unique_ptr<Expr> value, int sourceLine);

    const Expr* getExpression() const;
    void print(int indentLevel) const override;
};

class ReturnStmt : public Stmt
{
private:
    unique_ptr<Expr> expression;

public:
    ReturnStmt(unique_ptr<Expr> value, int sourceLine);

    const Expr* getExpression() const;
    void print(int indentLevel) const override;
};

class BlockStmt : public Stmt
{
private:
    vector<unique_ptr<Stmt>> statements;

public:
    BlockStmt(vector<unique_ptr<Stmt>> blockStatements, int sourceLine);

    const vector<unique_ptr<Stmt>>& getStatements() const;
    void print(int indentLevel) const override;
};

struct FunctionParameter
{
    string typeName;
    string name;
    int line;
};

class FunctionStmt : public Stmt
{
private:
    string returnType;
    string name;
    vector<FunctionParameter> parameters;
    unique_ptr<BlockStmt> body;

public:
    FunctionStmt(const string& declaredReturnType,
                 const string& functionName,
                 vector<FunctionParameter> functionParameters,
                 unique_ptr<BlockStmt> functionBody,
                 int sourceLine);

    const string& getReturnType() const;
    const string& getName() const;
    const vector<FunctionParameter>& getParameters() const;
    const BlockStmt* getBody() const;
    void print(int indentLevel) const override;
};

class IfStmt : public Stmt
{
private:
    unique_ptr<Expr> condition;
    unique_ptr<Stmt> thenBranch;
    unique_ptr<Stmt> elseBranch;

public:
    IfStmt(unique_ptr<Expr> conditionExpression,
           unique_ptr<Stmt> thenStatement,
           unique_ptr<Stmt> elseStatement,
           int sourceLine);

    const Expr* getCondition() const;
    const Stmt* getThenBranch() const;
    const Stmt* getElseBranch() const;
    void print(int indentLevel) const override;
};

class WhileStmt : public Stmt
{
private:
    unique_ptr<Expr> condition;
    unique_ptr<Stmt> body;

public:
    WhileStmt(unique_ptr<Expr> conditionExpression,
              unique_ptr<Stmt> bodyStatement,
              int sourceLine);

    const Expr* getCondition() const;
    const Stmt* getBody() const;
    void print(int indentLevel) const override;
};

class Program
{
private:
    vector<unique_ptr<Stmt>> statements;

public:
    explicit Program(vector<unique_ptr<Stmt>> programStatements);

    const vector<unique_ptr<Stmt>>& getStatements() const;
    void print() const;
};

#endif
