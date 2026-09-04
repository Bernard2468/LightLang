#ifndef PARSER_H
#define PARSER_H

#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "frontend/AST.h"
#include "frontend/Token.h"

using namespace std;

class ParserError : public runtime_error
{
public:
    // Location of the offending token, carried alongside the message so the
    // driver can render a source excerpt with a caret.
    int line;
    int column;

    explicit ParserError(const string& message, int errorLine = 0, int errorColumn = 0)
        : runtime_error(message), line(errorLine), column(errorColumn)
    {
    }
};

// One recorded syntax error. The parser recovers after each one so a single
// run can report every syntax error in the file rather than only the first.
struct ParseDiagnostic
{
    string message;
    int line;
    int column;
};

class Parser
{
private:
    // Guard against a damaged file producing endless cascading errors.
    static const size_t MAX_REPORTED_ERRORS = 20;

    vector<Token> tokens;
    size_t current;
    vector<ParseDiagnostic> errors;

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

    // Records a diagnostic, then skips tokens until a plausible statement
    // boundary so parsing can continue past the damaged construct.
    void recordError(const ParserError& parseError, const Token& token);
    void synchronize();

    unique_ptr<Stmt> declaration(bool allowFunction);
    unique_ptr<Stmt> functionDeclaration(int sourceLine);
    unique_ptr<Stmt> variableDeclaration(const Token& typeToken);
    unique_ptr<Stmt> statement();
    unique_ptr<Stmt> assignmentStatement();

    // Shared by plain assignments and by the 'for' header, which needs an
    // assignment that does not consume a trailing ';'.
    unique_ptr<Stmt> assignmentCore(bool expectSemicolon);

    // True when the cursor sits on IDENTIFIER followed by '=' or a compound
    // assignment operator.
    bool isAssignmentStart() const;

    // Maps '+=' to '+', '-=' to '-', and so on, for desugaring.
    Token compoundAssignmentOperator(const Token& compound) const;

    unique_ptr<Stmt> forStatement(int sourceLine);
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

    // Parses the whole token stream, recovering from syntax errors. The
    // returned Program is only meaningful when hasErrors() is false.
    Program parse();

    bool hasErrors() const;
    const vector<ParseDiagnostic>& getErrors() const;
};

#endif
