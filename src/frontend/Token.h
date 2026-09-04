#ifndef TOKEN_H
#define TOKEN_H

#include <string>

using namespace std;

enum class TokenType
{
    // Keywords
    INT,
    FLOAT,
    BOOL,
    STRING,
    PRINT,
    IF,
    ELSE,
    WHILE,
    FUNC,
    RETURN,
    TRUE,
    FALSE,
    FOR,
    BREAK,
    CONTINUE,

    // Literals and identifiers
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,

    // Operators
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    ASSIGN,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    MULTIPLY_ASSIGN,
    DIVIDE_ASSIGN,
    MODULO_ASSIGN,
    EQUAL,
    NOT_EQUAL,
    GREATER,
    LESS,
    GREATER_EQUAL,
    LESS_EQUAL,
    AND,
    OR,
    NOT,

    // Delimiters
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    SEMICOLON,
    COMMA,

    END_OF_FILE,
    UNKNOWN
};

struct Token
{
    TokenType type;
    string lexeme;
    int line;

    // 1-based offset of the token's first character within its line. Used to
    // point a caret at the exact token in diagnostics. Zero means "unknown",
    // in which case the caret line is omitted.
    int column;

    Token(TokenType tokenType, string tokenLexeme, int tokenLine, int tokenColumn = 0)
        : type(tokenType),
          lexeme(tokenLexeme),
          line(tokenLine),
          column(tokenColumn)
    {
    }
};

string tokenTypeToString(TokenType type);

#endif
