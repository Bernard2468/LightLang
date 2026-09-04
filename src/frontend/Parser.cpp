#include "frontend/Parser.h"
#include <sstream>
#include <utility>

using namespace std;

Parser::Parser(const vector<Token>& tokenList) : tokens(tokenList), current(0) {}
bool Parser::isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }
const Token& Parser::peek() const { return tokens[current]; }
const Token& Parser::previous() const { return tokens[current - 1]; }
const Token& Parser::advance()
{
    if (!isAtEnd()) current++;
    return previous();
}
bool Parser::check(TokenType type) const
{
    if (isAtEnd()) return type == TokenType::END_OF_FILE;
    return peek().type == type;
}
bool Parser::checkNext(TokenType type) const
{
    if (current + 1 >= tokens.size()) return false;
    return tokens[current + 1].type == type;
}
bool Parser::match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}
bool Parser::matchAny(initializer_list<TokenType> types)
{
    for (TokenType type : types)
    {
        if (check(type))
        {
            advance();
            return true;
        }
    }
    return false;
}
Token Parser::consume(TokenType type, const string& message)
{
    if (check(type)) return advance();
    throw error(peek(), message);
}
Token Parser::consumeType(const string& message)
{
    if (matchAny({TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING}))
        return previous();
    throw error(peek(), message);
}
ParserError Parser::error(const Token& token, const string& message) const
{
    stringstream output;
    output << "Syntax error at line " << token.line;
    if (token.column > 0) output << ", column " << token.column;
    if (token.type == TokenType::END_OF_FILE) output << " at end of file";
    else output << " near '" << token.lexeme << "'";
    output << ": " << message;
    return ParserError(output.str(), token.line, token.column);
}

void Parser::recordError(const ParserError& parseError, const Token& token)
{
    int line = parseError.line > 0 ? parseError.line : token.line;
    int column = parseError.line > 0 ? parseError.column : token.column;
    errors.push_back(ParseDiagnostic{parseError.what(), line, column});
}

void Parser::synchronize()
{
    // Always consume at least one token so that recovery cannot loop forever
    // on a construct the parser rejected without advancing.
    advance();

    while (!isAtEnd())
    {
        if (previous().type == TokenType::SEMICOLON) return;

        switch (peek().type)
        {
            case TokenType::FUNC:
            case TokenType::INT:
            case TokenType::FLOAT:
            case TokenType::BOOL:
            case TokenType::STRING:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::BREAK:
            case TokenType::CONTINUE:
            case TokenType::PRINT:
            case TokenType::RETURN:
                return;
            default:
                break;
        }

        advance();
    }
}

bool Parser::hasErrors() const { return !errors.empty(); }
const vector<ParseDiagnostic>& Parser::getErrors() const { return errors; }

Program Parser::parse()
{
    vector<unique_ptr<Stmt>> statements;

    while (!isAtEnd())
    {
        size_t positionBefore = current;

        try
        {
            statements.push_back(declaration(true));
        }
        catch (const ParserError& parseError)
        {
            recordError(parseError, peek());
            synchronize();

            if (errors.size() >= MAX_REPORTED_ERRORS)
            {
                break;
            }
        }

        // Defensive guarantee of forward progress: no production should leave
        // the cursor untouched, but a stall here would hang the compiler.
        if (current == positionBefore && !isAtEnd())
        {
            advance();
        }
    }

    return Program(move(statements));
}

unique_ptr<Stmt> Parser::declaration(bool allowFunction)
{
    if (match(TokenType::FUNC))
    {
        if (!allowFunction)
            throw error(previous(), "Function declarations are only allowed at top level.");
        return functionDeclaration(previous().line);
    }

    if (matchAny({TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING}))
        return variableDeclaration(previous());

    return statement();
}

unique_ptr<Stmt> Parser::functionDeclaration(int sourceLine)
{
    Token returnType = consumeType("Expected a return type after 'func'.");
    Token name = consume(TokenType::IDENTIFIER, "Expected a function name.");
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name.");

    vector<FunctionParameter> parameters;
    if (!check(TokenType::RIGHT_PAREN))
    {
        do
        {
            Token parameterType = consumeType("Expected a parameter type.");
            Token parameterName = consume(TokenType::IDENTIFIER,
                                          "Expected a parameter name after its type.");
            parameters.push_back({parameterType.lexeme,
                                  parameterName.lexeme,
                                  parameterType.line});
        }
        while (match(TokenType::COMMA));
    }

    consume(TokenType::RIGHT_PAREN, "Expected ')' after function parameters.");
    Token leftBrace = consume(TokenType::LEFT_BRACE,
                              "Expected '{' before function body.");
    unique_ptr<BlockStmt> body = blockStatement(leftBrace.line);

    return make_unique<FunctionStmt>(returnType.lexeme,
                                     name.lexeme,
                                     move(parameters),
                                     move(body),
                                     sourceLine);
}

unique_ptr<Stmt> Parser::variableDeclaration(const Token& typeToken)
{
    Token name = consume(TokenType::IDENTIFIER,
                         "Expected a variable name after the type.");
    unique_ptr<Expr> initializer = nullptr;
    if (match(TokenType::ASSIGN)) initializer = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    return make_unique<VarDeclStmt>(typeToken.lexeme, name.lexeme,
                                    move(initializer), typeToken.line);
}

unique_ptr<Stmt> Parser::statement()
{
    if (match(TokenType::PRINT)) return printStatement(previous().line);
    if (match(TokenType::RETURN)) return returnStatement(previous().line);
    if (match(TokenType::IF)) return ifStatement(previous().line);
    if (match(TokenType::WHILE)) return whileStatement(previous().line);
    if (match(TokenType::FOR)) return forStatement(previous().line);
    if (match(TokenType::LEFT_BRACE)) return blockStatement(previous().line);

    if (match(TokenType::BREAK))
    {
        int line = previous().line;
        consume(TokenType::SEMICOLON, "Expected ';' after 'break'.");
        return make_unique<BreakStmt>(line);
    }

    if (match(TokenType::CONTINUE))
    {
        int line = previous().line;
        consume(TokenType::SEMICOLON, "Expected ';' after 'continue'.");
        return make_unique<ContinueStmt>(line);
    }

    if (isAssignmentStart()) return assignmentStatement();
    throw error(peek(), "Expected a valid statement.");
}

unique_ptr<Stmt> Parser::assignmentStatement()
{
    return assignmentCore(true);
}

bool Parser::isAssignmentStart() const
{
    if (!check(TokenType::IDENTIFIER)) return false;
    if (current + 1 >= tokens.size()) return false;

    switch (tokens[current + 1].type)
    {
        case TokenType::ASSIGN:
        case TokenType::PLUS_ASSIGN:
        case TokenType::MINUS_ASSIGN:
        case TokenType::MULTIPLY_ASSIGN:
        case TokenType::DIVIDE_ASSIGN:
        case TokenType::MODULO_ASSIGN:
            return true;
        default:
            return false;
    }
}

Token Parser::compoundAssignmentOperator(const Token& compound) const
{
    switch (compound.type)
    {
        case TokenType::PLUS_ASSIGN:
            return Token(TokenType::PLUS, "+", compound.line, compound.column);
        case TokenType::MINUS_ASSIGN:
            return Token(TokenType::MINUS, "-", compound.line, compound.column);
        case TokenType::MULTIPLY_ASSIGN:
            return Token(TokenType::MULTIPLY, "*", compound.line, compound.column);
        case TokenType::DIVIDE_ASSIGN:
            return Token(TokenType::DIVIDE, "/", compound.line, compound.column);
        case TokenType::MODULO_ASSIGN:
            return Token(TokenType::MODULO, "%", compound.line, compound.column);
        default:
            throw error(compound, "Unsupported compound assignment operator.");
    }
}

unique_ptr<Stmt> Parser::assignmentCore(bool expectSemicolon)
{
    Token name = consume(TokenType::IDENTIFIER, "Expected a variable name.");
    unique_ptr<Expr> value;

    if (match(TokenType::ASSIGN))
    {
        value = expression();
    }
    else if (matchAny({TokenType::PLUS_ASSIGN,
                       TokenType::MINUS_ASSIGN,
                       TokenType::MULTIPLY_ASSIGN,
                       TokenType::DIVIDE_ASSIGN,
                       TokenType::MODULO_ASSIGN}))
    {
        // Desugar 'x += e' into 'x = x + e'. The target is a plain variable,
        // so re-reading it is free of side effects, and the existing type
        // rules for the binary operator apply unchanged.
        Token compound = previous();
        Token binaryOperator = compoundAssignmentOperator(compound);

        unique_ptr<Expr> target = make_unique<VariableExpr>(name.lexeme, name.line);
        unique_ptr<Expr> operand = expression();
        value = make_unique<BinaryExpr>(move(target), binaryOperator, move(operand));
    }
    else
    {
        throw error(peek(), "Expected '=' in assignment statement.");
    }

    if (expectSemicolon)
    {
        consume(TokenType::SEMICOLON, "Expected ';' after assignment.");
    }

    return make_unique<AssignmentStmt>(name.lexeme, move(value), name.line);
}

unique_ptr<Stmt> Parser::forStatement(int sourceLine)
{
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'for'.");

    unique_ptr<Stmt> initializer;
    if (matchAny({TokenType::INT, TokenType::FLOAT,
                  TokenType::BOOL, TokenType::STRING}))
    {
        initializer = variableDeclaration(previous());
    }
    else if (isAssignmentStart())
    {
        initializer = assignmentCore(true);
    }
    else
    {
        throw error(peek(),
                    "Expected a variable declaration or an assignment in the 'for' initializer.");
    }

    unique_ptr<Expr> condition = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after the 'for' condition.");

    if (!isAssignmentStart())
    {
        throw error(peek(), "Expected an assignment in the 'for' increment.");
    }

    unique_ptr<Stmt> increment = assignmentCore(false);
    consume(TokenType::RIGHT_PAREN, "Expected ')' after the 'for' clauses.");

    unique_ptr<Stmt> body = statement();

    return make_unique<ForStmt>(move(initializer), move(condition),
                                move(increment), move(body), sourceLine);
}

unique_ptr<Stmt> Parser::printStatement(int sourceLine)
{
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'print'.");
    unique_ptr<Expr> value = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after value in print statement.");
    consume(TokenType::SEMICOLON, "Expected ';' after print statement.");
    return make_unique<PrintStmt>(move(value), sourceLine);
}

unique_ptr<Stmt> Parser::returnStatement(int sourceLine)
{
    unique_ptr<Expr> value = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after return value.");
    return make_unique<ReturnStmt>(move(value), sourceLine);
}

unique_ptr<Stmt> Parser::ifStatement(int sourceLine)
{
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'.");
    unique_ptr<Expr> condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition.");
    unique_ptr<Stmt> thenBranch = statement();
    unique_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenType::ELSE)) elseBranch = statement();
    return make_unique<IfStmt>(move(condition), move(thenBranch),
                               move(elseBranch), sourceLine);
}

unique_ptr<Stmt> Parser::whileStatement(int sourceLine)
{
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'.");
    unique_ptr<Expr> condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition.");
    unique_ptr<Stmt> body = statement();
    return make_unique<WhileStmt>(move(condition), move(body), sourceLine);
}

unique_ptr<BlockStmt> Parser::blockStatement(int sourceLine)
{
    vector<unique_ptr<Stmt>> statements;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        statements.push_back(declaration(false));
    consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
    return make_unique<BlockStmt>(move(statements), sourceLine);
}

unique_ptr<Expr> Parser::expression() { return logicalOr(); }
unique_ptr<Expr> Parser::logicalOr()
{
    unique_ptr<Expr> expr = logicalAnd();
    while (match(TokenType::OR))
    {
        Token op = previous();
        unique_ptr<Expr> right = logicalAnd();
        expr = make_unique<BinaryExpr>(move(expr), op, move(right));
    }
    return expr;
}
unique_ptr<Expr> Parser::logicalAnd()
{
    unique_ptr<Expr> expr = equality();
    while (match(TokenType::AND))
    {
        Token op = previous();
        unique_ptr<Expr> right = equality();
        expr = make_unique<BinaryExpr>(move(expr), op, move(right));
    }
    return expr;
}
unique_ptr<Expr> Parser::equality()
{
    unique_ptr<Expr> expr = comparison();
    while (matchAny({TokenType::EQUAL, TokenType::NOT_EQUAL}))
    {
        Token op = previous();
        unique_ptr<Expr> right = comparison();
        expr = make_unique<BinaryExpr>(move(expr), op, move(right));
    }
    return expr;
}
unique_ptr<Expr> Parser::comparison()
{
    unique_ptr<Expr> expr = term();
    while (matchAny({TokenType::GREATER, TokenType::GREATER_EQUAL,
                     TokenType::LESS, TokenType::LESS_EQUAL}))
    {
        Token op = previous();
        unique_ptr<Expr> right = term();
        expr = make_unique<BinaryExpr>(move(expr), op, move(right));
    }
    return expr;
}
unique_ptr<Expr> Parser::term()
{
    unique_ptr<Expr> expr = factor();
    while (matchAny({TokenType::PLUS, TokenType::MINUS}))
    {
        Token op = previous();
        unique_ptr<Expr> right = factor();
        expr = make_unique<BinaryExpr>(move(expr), op, move(right));
    }
    return expr;
}
unique_ptr<Expr> Parser::factor()
{
    unique_ptr<Expr> expr = unary();
    while (matchAny({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO}))
    {
        Token op = previous();
        unique_ptr<Expr> right = unary();
        expr = make_unique<BinaryExpr>(move(expr), op, move(right));
    }
    return expr;
}
unique_ptr<Expr> Parser::unary()
{
    if (matchAny({TokenType::NOT, TokenType::MINUS, TokenType::PLUS}))
    {
        Token op = previous();
        unique_ptr<Expr> right = unary();
        return make_unique<UnaryExpr>(op, move(right));
    }
    return primary();
}
unique_ptr<Expr> Parser::primary()
{
    if (matchAny({TokenType::INTEGER_LITERAL, TokenType::FLOAT_LITERAL,
                  TokenType::STRING_LITERAL, TokenType::TRUE, TokenType::FALSE}))
    {
        return make_unique<LiteralExpr>(previous().lexeme,
                                        previous().type,
                                        previous().line);
    }

    if (match(TokenType::IDENTIFIER))
    {
        Token name = previous();
        if (match(TokenType::LEFT_PAREN))
        {
            vector<unique_ptr<Expr>> arguments;
            if (!check(TokenType::RIGHT_PAREN))
            {
                do
                {
                    arguments.push_back(expression());
                }
                while (match(TokenType::COMMA));
            }
            consume(TokenType::RIGHT_PAREN, "Expected ')' after function arguments.");
            return make_unique<CallExpr>(name.lexeme, move(arguments), name.line);
        }
        return make_unique<VariableExpr>(name.lexeme, name.line);
    }

    if (match(TokenType::LEFT_PAREN))
    {
        int sourceLine = previous().line;
        unique_ptr<Expr> expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
        return make_unique<GroupingExpr>(move(expr), sourceLine);
    }

    throw error(peek(), "Expected an expression.");
}
