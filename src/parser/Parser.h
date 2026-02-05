#pragma once

#include <memory>
#include <vector>
#include <string>
#include <stdexcept>

#include "lexer/Token.h"
#include "ast/ASTCore.h"
#include "ast/ASTExpressions.h"
#include "ast/ASTStatements.h"
#include "ast/ASTTypes.h"
#include "ast/ASTDeclarations.h"

namespace obould {

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message, int line, int column)
        : std::runtime_error(message), line_(line), column_(column) {}

    int line() const { return line_; }
    int column() const { return column_; }

private:
    int line_;
    int column_;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<Module> parse();

    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }

private:
    // Token navigation
    const Token& peek() const;
    const Token& peekNext() const;
    const Token& previous() const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token expect(TokenType type, const std::string& message);

    // Error handling
    ParseError error(const std::string& message);
    ParseError error(const Token& token, const std::string& message);
    void synchronize();

    // Module parsing
    std::unique_ptr<Module> parseModule();
    std::vector<std::unique_ptr<Import>> parseImportList();
    std::unique_ptr<Import> parseImport();

    // Declaration parsing
    std::unique_ptr<DeclarationsBlock> parseDeclarationSequence();
    std::unique_ptr<ConstantDeclarations> parseConstantDeclarations();
    std::unique_ptr<ConstantDeclaration> parseConstantDeclaration();
    std::unique_ptr<TypeDeclarations> parseTypeDeclarations();
    std::unique_ptr<TypeDeclaration> parseTypeDeclaration();
    std::unique_ptr<VariableDeclarations> parseVariableDeclarations();
    std::unique_ptr<VariableDeclaration> parseVariableDeclaration();
    std::vector<std::unique_ptr<VariableDeclaration>> parseIdentifierListWithType();

    // Procedure parsing
    std::unique_ptr<ProcedureDeclaration> parseProcedureDeclaration();
    std::vector<std::unique_ptr<ProcedureParameter>> parseFormalParameters();
    std::unique_ptr<ProcedureParameter> parseFormalParameterSection();
    std::unique_ptr<Type> parseReturnType();

    // Type parsing
    std::unique_ptr<Type> parseType();
    std::unique_ptr<IdentifierType> parseIdentifierType();
    std::unique_ptr<Type> parseArrayType(std::unique_ptr<Type> elementType);
    std::unique_ptr<StructType> parseStructType();
    std::unique_ptr<PointerType> parsePointerType();
    std::unique_ptr<Type> parseProcedureType();

    // Statement parsing
    std::unique_ptr<StatementsBlock> parseStatementSequence();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Statement> parseAssignmentOrProcedureCall();
    std::unique_ptr<IfStatement> parseIfStatement();
    std::unique_ptr<WhileStatement> parseWhileStatement();
    std::unique_ptr<DoWhileStatement> parseDoWhileStatement();
    std::unique_ptr<ForStatement> parseForStatement();
    std::unique_ptr<SwitchStatement> parseSwitchStatement();
    std::unique_ptr<SwitchCase> parseSwitchCase();
    std::unique_ptr<CaseLabel> parseCaseLabel();
    std::unique_ptr<Statement> parseConditionBody();

    // Expression parsing (operator precedence)
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseSimpleExpression();
    std::unique_ptr<Expression> parseTerm();
    std::unique_ptr<Expression> parseFactor();
    std::unique_ptr<Expression> parseDesignator();
    std::unique_ptr<ProcedureCall> parseProcedureCall(std::unique_ptr<Expression> callee);

    // Helper for identifier declaration (with optional export)
    std::pair<std::string, bool> parseIdentifierDeclaration();
    std::pair<std::string, std::string> parseQualifiedIdentifier();

    std::vector<Token> tokens_;
    size_t current_ = 0;
    std::vector<std::string> errors_;
};

} // namespace obould
