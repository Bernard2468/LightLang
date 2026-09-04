#include "Lexer.h"
#include <cctype>

using namespace std;

Lexer::Lexer(const string& sourceCode)
    : source(sourceCode), current(0), line(1)
{
    keywords = {
        {"int", TokenType::INT},
        {"float", TokenType::FLOAT},
        {"bool", TokenType::BOOL},
        {"string", TokenType::STRING},
        {"print", TokenType::PRINT},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"func", TokenType::FUNC},
        {"return", TokenType::RETURN},
        {"true", TokenType::TRUE},
        {"false", TokenType::FALSE}
    };
}

bool Lexer::isAtEnd() const
{
    return current >= source.length();
}

char Lexer::advance()
{
    return source[current++];
}

char Lexer::peek() const
{
    if (isAtEnd())
    {
        return '\0';
    }

    return source[current];
}

char Lexer::peekNext() const
{
    if (current + 1 >= source.length())
    {
        return '\0';
    }

    return source[current + 1];
}

bool Lexer::match(char expected)
{
    if (isAtEnd() || source[current] != expected)
    {
        return false;
    }

    current++;
    return true;
}

void Lexer::skipWhitespaceAndComments()
{
    while (!isAtEnd())
    {
        char c = peek();

        if (c == ' ' || c == '\r' || c == '\t')
        {
            advance();
        }
        else if (c == '\n')
        {
            line++;
            advance();
        }
        else if (c == '/' && peekNext() == '/')
        {
            while (peek() != '\n' && !isAtEnd())
            {
                advance();
            }
        }
        else
        {
            break;
        }
    }
}

Token Lexer::identifier()
{
    size_t start = current - 1;

    while (isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
    {
        advance();
    }

    string text = source.substr(start, current - start);

    auto keyword = keywords.find(text);
    if (keyword != keywords.end())
    {
        return Token(keyword->second, text, line);
    }

    return Token(TokenType::IDENTIFIER, text, line);
}

Token Lexer::number()
{
    size_t start = current - 1;
    bool isFloat = false;

    while (isdigit(static_cast<unsigned char>(peek())))
    {
        advance();
    }

    if (peek() == '.' && isdigit(static_cast<unsigned char>(peekNext())))
    {
        isFloat = true;
        advance();

        while (isdigit(static_cast<unsigned char>(peek())))
        {
            advance();
        }
    }

    string text = source.substr(start, current - start);

    if (isFloat)
    {
        return Token(TokenType::FLOAT_LITERAL, text, line);
    }

    return Token(TokenType::INTEGER_LITERAL, text, line);
}

Token Lexer::stringLiteral()
{
    size_t startLine = line;
    string value;

    while (!isAtEnd())
    {
        char character = advance();

        if (character == '"')
        {
            return Token(TokenType::STRING_LITERAL,
                         value,
                         static_cast<int>(startLine));
        }

        if (character == '\n')
        {
            line++;
            value += character;
            continue;
        }

        if (character != '\\')
        {
            value += character;
            continue;
        }

        if (isAtEnd())
        {
            break;
        }

        char escaped = advance();

        switch (escaped)
        {
            case '\\': value += '\\'; break;
            case '"': value += '"'; break;
            case 'n': value += '\n'; break;
            case 't': value += '\t'; break;
            case 'r': value += '\r'; break;
            default:
                value += '\\';
                value += escaped;
                break;
        }
    }

    return Token(TokenType::UNKNOWN,
                 "Unterminated string",
                 static_cast<int>(startLine));
}

Token Lexer::nextToken()
{
    skipWhitespaceAndComments();

    if (isAtEnd())
    {
        return Token(TokenType::END_OF_FILE, "", line);
    }

    char c = advance();

    if (isalpha(static_cast<unsigned char>(c)) || c == '_')
    {
        return identifier();
    }

    if (isdigit(static_cast<unsigned char>(c)))
    {
        return number();
    }

    switch (c)
    {
        case '+': return Token(TokenType::PLUS, "+", line);
        case '-': return Token(TokenType::MINUS, "-", line);
        case '*': return Token(TokenType::MULTIPLY, "*", line);
        case '/': return Token(TokenType::DIVIDE, "/", line);
        case '%': return Token(TokenType::MODULO, "%", line);

        case '=':
            if (match('=')) return Token(TokenType::EQUAL, "==", line);
            return Token(TokenType::ASSIGN, "=", line);

        case '!':
            if (match('=')) return Token(TokenType::NOT_EQUAL, "!=", line);
            return Token(TokenType::NOT, "!", line);

        case '>':
            if (match('=')) return Token(TokenType::GREATER_EQUAL, ">=", line);
            return Token(TokenType::GREATER, ">", line);

        case '<':
            if (match('=')) return Token(TokenType::LESS_EQUAL, "<=", line);
            return Token(TokenType::LESS, "<", line);

        case '&':
            if (match('&')) return Token(TokenType::AND, "&&", line);
            return Token(TokenType::UNKNOWN, "&", line);

        case '|':
            if (match('|')) return Token(TokenType::OR, "||", line);
            return Token(TokenType::UNKNOWN, "|", line);

        case '(':
            return Token(TokenType::LEFT_PAREN, "(", line);

        case ')':
            return Token(TokenType::RIGHT_PAREN, ")", line);

        case '{':
            return Token(TokenType::LEFT_BRACE, "{", line);

        case '}':
            return Token(TokenType::RIGHT_BRACE, "}", line);

        case ';':
            return Token(TokenType::SEMICOLON, ";", line);

        case ',':
            return Token(TokenType::COMMA, ",", line);

        case '"':
            return stringLiteral();

        default:
            return Token(TokenType::UNKNOWN, string(1, c), line);
    }
}

vector<Token> Lexer::tokenize()
{
    vector<Token> tokens;

    while (true)
    {
        Token token = nextToken();
        tokens.push_back(token);

        if (token.type == TokenType::END_OF_FILE)
        {
            break;
        }
    }

    return tokens;
}
