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

    Token(TokenType tokenType, string tokenLexeme, int tokenLine)
        : type(tokenType), lexeme(tokenLexeme), line(tokenLine)
    {
    }
};

string tokenTypeToString(TokenType type);

#endif
