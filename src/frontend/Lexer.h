#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "frontend/Token.h"

using namespace std;

class Lexer
{
private:
    string source;
    size_t current;
    int line;

    // Index just past the most recent newline, so that a token's column is
    // (tokenStart - lineStart + 1). Set once per token by nextToken().
    size_t lineStart;
    int tokenColumn;

    unordered_map<string, TokenType> keywords;

    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);

    void skipWhitespaceAndComments();
    Token identifier();
    Token number();
    Token stringLiteral();

public:
    explicit Lexer(const string& sourceCode);

    Token nextToken();
    vector<Token> tokenize();
};

#endif
