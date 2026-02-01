#include "Lexer.h"
#include <sstream>

namespace obould {

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"switch", TokenType::KW_SWITCH},
    {"case",   TokenType::KW_CASE},
    {"if",     TokenType::KW_IF},
    {"else",   TokenType::KW_ELSE},
    {"elif",   TokenType::KW_ELIF},
    {"while",  TokenType::KW_WHILE},
    {"do",     TokenType::KW_DO},
    {"for",    TokenType::KW_FOR},
    {"fn",     TokenType::KW_FN},
    {"module", TokenType::KW_MODULE},
    {"return", TokenType::KW_RETURN},
    {"var",    TokenType::KW_VAR},
    {"type",   TokenType::KW_TYPE},
    {"const",  TokenType::KW_CONST},
    {"true",   TokenType::KW_TRUE},
    {"false",  TokenType::KW_FALSE},
    {"is",     TokenType::KW_IS},
    {"nil",    TokenType::KW_NIL},
    {"div",    TokenType::KW_DIV},
    {"mod",    TokenType::KW_MOD},
    {"struct", TokenType::KW_STRUCT},
    {"export", TokenType::KW_EXPORT},
    {"import", TokenType::KW_IMPORT},
    {"void",   TokenType::KW_VOID},
};

Lexer::Lexer(std::string source)
    : source_(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        Token token = nextToken();
        tokens.push_back(token);
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
    }

    if (tokens.empty() || tokens.back().type != TokenType::END_OF_FILE) {
        tokens.push_back(makeToken(TokenType::END_OF_FILE));
    }

    return tokens;
}

Token Lexer::nextToken() {
    skipWhitespace();

    start_ = current_;
    tokenStartColumn_ = column_;

    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE);
    }

    return scanToken();
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.length();
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source_[current_] != expected) return false;
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n':
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    // Single-line comment
                    while (peek() != '\n' && !isAtEnd()) {
                        advance();
                    }
                } else if (peekNext() == '*') {
                    // Multi-line comment
                    advance(); // consume '/'
                    advance(); // consume '*'
                    while (!isAtEnd()) {
                        if (peek() == '*' && peekNext() == '/') {
                            advance(); // consume '*'
                            advance(); // consume '/'
                            break;
                        }
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token Lexer::scanToken() {
    char c = advance();

    if (isAlpha(c)) {
        return identifier();
    }

    if (isDigit(c)) {
        return number();
    }

    switch (c) {
        case '"': return string();

        case '(': return makeToken(TokenType::LPAREN);
        case ')': return makeToken(TokenType::RPAREN);
        case '{': return makeToken(TokenType::LBRACE);
        case '}': return makeToken(TokenType::RBRACE);
        case '[': return makeToken(TokenType::LBRACKET);
        case ']': return makeToken(TokenType::RBRACKET);
        case ';': return makeToken(TokenType::SEMICOLON);
        case ',': return makeToken(TokenType::COMMA);
        case ':': return makeToken(TokenType::COLON);

        case '.':
            if (match('.')) {
                return makeToken(TokenType::OP_DOTDOT);
            }
            return makeToken(TokenType::DOT);

        case '+': return makeToken(TokenType::OP_PLUS);
        case '-':
            if (match('>')) {
                return makeToken(TokenType::OP_ARROW);
            }
            return makeToken(TokenType::OP_MINUS);
        case '*': return makeToken(TokenType::OP_STAR);
        case '/': return makeToken(TokenType::OP_SLASH);

        case '=':
            if (match('=')) {
                return makeToken(TokenType::OP_EQ);
            }
            return makeToken(TokenType::OP_ASSIGN);

        case '!':
            if (match('=')) {
                return makeToken(TokenType::OP_NE);
            }
            return makeToken(TokenType::OP_NOT);

        case '<':
            if (match('=')) {
                return makeToken(TokenType::OP_LE);
            }
            return makeToken(TokenType::OP_LT);

        case '>':
            if (match('=')) {
                return makeToken(TokenType::OP_GE);
            }
            return makeToken(TokenType::OP_GT);

        case '&':
            if (match('&')) {
                return makeToken(TokenType::OP_AND);
            }
            return makeToken(TokenType::AMPERSAND);

        case '|':
            if (match('|')) {
                return makeToken(TokenType::OP_OR);
            }
            return errorToken("Unexpected character '|'. Did you mean '||'?");
    }

    std::ostringstream oss;
    oss << "Unexpected character: '" << c << "'";
    return errorToken(oss.str());
}

Token Lexer::identifier() {
    while (isAlphaNumeric(peek())) {
        advance();
    }

    std::string text = source_.substr(start_, current_ - start_);

    auto it = keywords_.find(text);
    if (it != keywords_.end()) {
        return makeToken(it->second, text);
    }

    return makeToken(TokenType::IDENTIFIER, text);
}

Token Lexer::number() {
    bool isFloat = false;

    // Check for hex number: 0x...
    if (source_[start_] == '0' && peek() == 'x') {
        advance(); // consume 'x'
        if (!isHexDigit(peek())) {
            return errorToken("Invalid hex number: expected hex digit after '0x'");
        }
        while (isHexDigit(peek())) {
            advance();
        }
        std::string text = source_.substr(start_, current_ - start_);
        return makeToken(TokenType::INTEGER, text);
    }

    // Decimal integer part
    while (isDigit(peek())) {
        advance();
    }

    // Check for decimal point (float)
    if (peek() == '.' && peekNext() != '.') {
        // Make sure it's not ".." operator
        isFloat = true;
        advance(); // consume '.'
        while (isDigit(peek())) {
            advance();
        }
    }

    // Check for exponent
    if (peek() == 'E' || peek() == 'e') {
        isFloat = true;
        advance(); // consume 'E'
        if (peek() == '+' || peek() == '-') {
            advance();
        }
        if (!isDigit(peek())) {
            return errorToken("Invalid number: expected digit after exponent");
        }
        while (isDigit(peek())) {
            advance();
        }
    }

    std::string text = source_.substr(start_, current_ - start_);
    return makeToken(isFloat ? TokenType::FLOAT : TokenType::INTEGER, text);
}

Token Lexer::string() {
    std::string value;

    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            return errorToken("Unterminated string: newline in string literal");
        }
        if (peek() == '\\') {
            advance(); // consume backslash
            if (isAtEnd()) {
                return errorToken("Unterminated string: escape at end of file");
            }
            char escaped = advance();
            switch (escaped) {
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                default:
                    std::ostringstream oss;
                    oss << "Invalid escape sequence: \\" << escaped;
                    return errorToken(oss.str());
            }
        } else {
            value += advance();
        }
    }

    if (isAtEnd()) {
        return errorToken("Unterminated string");
    }

    advance();

    std::string lexeme = source_.substr(start_, current_ - start_);
    return makeToken(TokenType::STRING, lexeme);
}

Token Lexer::makeToken(TokenType type) {
    std::string lexeme = source_.substr(start_, current_ - start_);
    return Token(type, lexeme, line_, tokenStartColumn_);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) {
    return Token(type, lexeme, line_, tokenStartColumn_);
}

Token Lexer::errorToken(const std::string& message) {
    std::ostringstream oss;
    oss << "Error at line " << line_ << ", column " << tokenStartColumn_ << ": " << message;
    errors_.push_back(oss.str());
    return Token(TokenType::ERROR, message, line_, tokenStartColumn_);
}

bool Lexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isHexDigit(char c) {
    return isDigit(c) ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

} // namespace obould
