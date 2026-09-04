#ifndef PARSER_H
#define PARSER_H

#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "AST.h"
#include "Token.h"

using namespace std;

class ParserError : public runtime_error
{
public:
    explicit ParserError(const string& message) : runtime_error(message) {}
};

class Parser
{
private:
    vector<Token> tokens;
    size_t current;

    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();

    bool check(TokenType type) const;
    bool checkNext(TokenType type) const;
    bool match(TokenType type);
    bool matchAny(initializer_list<TokenType> types);

    Token consume(TokenType type, const string& message);
    Token consumeType(const string& message);
    ParserError error(const Token& token, const string& message) const;

    unique_ptr<Stmt> declaration(bool allowFunction);
    unique_ptr<Stmt> functionDeclaration(int sourceLine);
    unique_ptr<Stmt> variableDeclaration(const Token& typeToken);
    unique_ptr<Stmt> statement();
    unique_ptr<Stmt> assignmentStatement();
    unique_ptr<Stmt> printStatement(int sourceLine);
    unique_ptr<Stmt> returnStatement(int sourceLine);
    unique_ptr<Stmt> ifStatement(int sourceLine);
    unique_ptr<Stmt> whileStatement(int sourceLine);
    unique_ptr<BlockStmt> blockStatement(int sourceLine);

    unique_ptr<Expr> expression();
    unique_ptr<Expr> logicalOr();
    unique_ptr<Expr> logicalAnd();
    unique_ptr<Expr> equality();
    unique_ptr<Expr> comparison();
    unique_ptr<Expr> term();
    unique_ptr<Expr> factor();
    unique_ptr<Expr> unary();
    unique_ptr<Expr> primary();

public:
    explicit Parser(const vector<Token>& tokenList);
    Program parse();
};

#endif
