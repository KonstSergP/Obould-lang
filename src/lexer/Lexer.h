#pragma once

#include "Token.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace obould {

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> tokenize();
    Token nextToken();

    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }

private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    bool match(char expected);

    void skipWhitespace();
    void skipComment();

    Token scanToken();
    Token identifier();
    Token number();
    Token string();

    Token makeToken(TokenType type);
    Token makeToken(TokenType type, const std::string& lexeme);
    Token errorToken(const std::string& message);

    static bool isAlpha(char c);
    static bool isDigit(char c);
    static bool isHexDigit(char c);
    static bool isAlphaNumeric(char c);

    static const std::unordered_map<std::string, TokenType> keywords_;

    std::string source_;
    size_t start_ = 0;
    size_t current_ = 0;
    int line_ = 1;
    int column_ = 1;
    int tokenStartColumn_ = 1;

    std::vector<std::string> errors_;
};

} // namespace obould
