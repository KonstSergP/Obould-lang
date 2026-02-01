#pragma once

#include <string>
#include <ostream>

namespace obould {

enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,

    // Identifier
    IDENTIFIER,

    // Keywords
    KW_SWITCH,
    KW_CASE,
    KW_IF,
    KW_ELSE,
    KW_ELIF,
    KW_WHILE,
    KW_DO,
    KW_FOR,
    KW_FN,
    KW_MODULE,
    KW_RETURN,
    KW_VAR,
    KW_TYPE,
    KW_CONST,
    KW_TRUE,
    KW_FALSE,
    KW_IS,
    KW_NIL,
    KW_DIV,
    KW_MOD,
    KW_STRUCT,
    KW_EXPORT,
    KW_IMPORT,
    KW_VOID,

    // Operators
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_STAR,        // *
    OP_SLASH,       // /
    OP_EQ,          // ==
    OP_NE,          // !=
    OP_LT,          // <
    OP_LE,          // <=
    OP_GT,          // >
    OP_GE,          // >=
    OP_AND,         // &&
    OP_OR,          // ||
    OP_NOT,         // !
    OP_ASSIGN,      // =
    OP_ARROW,       // ->
    OP_DOTDOT,      // ..

    // Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    SEMICOLON,      // ;
    COMMA,          // ,
    DOT,            // .
    COLON,          // :
    AMPERSAND,      // &

    // Special
    END_OF_FILE,
    ERROR
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType t, std::string lex, int l, int c)
        : type(t), lexeme(std::move(lex)), line(l), column(c) {}
};

inline const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INTEGER:     return "INTEGER";
        case TokenType::FLOAT:       return "FLOAT";
        case TokenType::STRING:      return "STRING";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::KW_SWITCH:   return "KW_SWITCH";
        case TokenType::KW_CASE:     return "KW_CASE";
        case TokenType::KW_IF:       return "KW_IF";
        case TokenType::KW_ELSE:     return "KW_ELSE";
        case TokenType::KW_ELIF:     return "KW_ELIF";
        case TokenType::KW_WHILE:    return "KW_WHILE";
        case TokenType::KW_DO:       return "KW_DO";
        case TokenType::KW_FOR:      return "KW_FOR";
        case TokenType::KW_FN:       return "KW_FN";
        case TokenType::KW_MODULE:   return "KW_MODULE";
        case TokenType::KW_RETURN:   return "KW_RETURN";
        case TokenType::KW_VAR:      return "KW_VAR";
        case TokenType::KW_TYPE:     return "KW_TYPE";
        case TokenType::KW_CONST:    return "KW_CONST";
        case TokenType::KW_TRUE:     return "KW_TRUE";
        case TokenType::KW_FALSE:    return "KW_FALSE";
        case TokenType::KW_IS:       return "KW_IS";
        case TokenType::KW_NIL:      return "KW_NIL";
        case TokenType::KW_DIV:      return "KW_DIV";
        case TokenType::KW_MOD:      return "KW_MOD";
        case TokenType::KW_STRUCT:   return "KW_STRUCT";
        case TokenType::KW_EXPORT:   return "KW_EXPORT";
        case TokenType::KW_IMPORT:   return "KW_IMPORT";
        case TokenType::KW_VOID:     return "KW_VOID";
        case TokenType::OP_PLUS:     return "OP_PLUS";
        case TokenType::OP_MINUS:    return "OP_MINUS";
        case TokenType::OP_STAR:     return "OP_STAR";
        case TokenType::OP_SLASH:    return "OP_SLASH";
        case TokenType::OP_EQ:       return "OP_EQ";
        case TokenType::OP_NE:       return "OP_NE";
        case TokenType::OP_LT:       return "OP_LT";
        case TokenType::OP_LE:       return "OP_LE";
        case TokenType::OP_GT:       return "OP_GT";
        case TokenType::OP_GE:       return "OP_GE";
        case TokenType::OP_AND:      return "OP_AND";
        case TokenType::OP_OR:       return "OP_OR";
        case TokenType::OP_NOT:      return "OP_NOT";
        case TokenType::OP_ASSIGN:   return "OP_ASSIGN";
        case TokenType::OP_ARROW:    return "OP_ARROW";
        case TokenType::OP_DOTDOT:   return "OP_DOTDOT";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::LBRACE:      return "LBRACE";
        case TokenType::RBRACE:      return "RBRACE";
        case TokenType::LBRACKET:    return "LBRACKET";
        case TokenType::RBRACKET:    return "RBRACKET";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::COMMA:       return "COMMA";
        case TokenType::DOT:         return "DOT";
        case TokenType::COLON:       return "COLON";
        case TokenType::AMPERSAND:   return "AMPERSAND";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR:       return "ERROR";
    }
    return "UNKNOWN";
}

inline std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << tokenTypeToString(token.type) << " \"" << token.lexeme
       << "\" [" << token.line << ":" << token.column << "]";
    return os;
}

} // namespace obould
