#include "Parser.h"
#include <sstream>
#include <cstdlib>

namespace obould {

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)) {}

// ============================================================================
// Token Navigation
// ============================================================================

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::peekNext() const {
    if (current_ + 1 >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[current_ + 1];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current_++;
    }
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    throw error(peek(), message);
}

// ============================================================================
// Error Handling
// ============================================================================

ParseError Parser::error(const std::string& message) {
    return error(peek(), message);
}

ParseError Parser::error(const Token& token, const std::string& message) {
    std::ostringstream oss;
    oss << "[" << token.line << ":" << token.column << "] Error at '"
        << token.lexeme << "': " << message;
    errors_.push_back(oss.str());
    return ParseError(message, token.line, token.column);
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        if (previous().type == TokenType::RBRACE) return;

        switch (peek().type) {
            case TokenType::KW_FN:
            case TokenType::KW_VAR:
            case TokenType::KW_CONST:
            case TokenType::KW_TYPE:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
            case TokenType::KW_MODULE:
                return;
            default:
                break;
        }
        advance();
    }
}

// ============================================================================
// Main Parse Entry Point
// ============================================================================

std::unique_ptr<Module> Parser::parse() {
    try {
        return parseModule();
    } catch (const ParseError& e) {
        return nullptr;
    }
}

// ============================================================================
// Module Parsing
// ============================================================================

std::unique_ptr<Module> Parser::parseModule() {
    expect(TokenType::KW_MODULE, "Expected 'module' at start of file");
    Token nameToken = expect(TokenType::IDENTIFIER, "Expected module name");
    std::string moduleName = nameToken.lexeme;

    std::vector<std::unique_ptr<Import>> imports;
    if (check(TokenType::KW_IMPORT)) {
        imports = parseImportList();
    }

    auto declarations = parseDeclarationSequence();

    std::vector<std::unique_ptr<ProcedureDeclaration>> procedures;
    while (check(TokenType::KW_FN)) {
        procedures.push_back(parseProcedureDeclaration());
    }

    if (!isAtEnd()) {
        throw error(peek(), "Unexpected token. Expected 'fn' or end of file");
    }

    return std::make_unique<Module>(
        moduleName,
        std::move(imports),
        std::move(declarations),
        std::move(procedures)
    );
}

std::vector<std::unique_ptr<Import>> Parser::parseImportList() {
    std::vector<std::unique_ptr<Import>> imports;

    expect(TokenType::KW_IMPORT, "Expected 'import'");

    do {
        imports.push_back(parseImport());
    } while (match(TokenType::COMMA));

    expect(TokenType::SEMICOLON, "Expected ';' after import list");

    return imports;
}

std::unique_ptr<Import> Parser::parseImport() {
    Token first = expect(TokenType::IDENTIFIER, "Expected module name in import");

    if (match(TokenType::OP_ASSIGN)) {
        Token realName = expect(TokenType::IDENTIFIER, "Expected module name after '='");
        return std::make_unique<Import>(first.lexeme, realName.lexeme);
    }

    return std::make_unique<Import>(first.lexeme, first.lexeme);
}

// ============================================================================
// Declaration Parsing
// ============================================================================

std::unique_ptr<DeclarationsBlock> Parser::parseDeclarationSequence() {
    std::unique_ptr<ConstantDeclarations> constants;
    std::unique_ptr<TypeDeclarations> types;
    std::unique_ptr<VariableDeclarations> variables;

    if (check(TokenType::KW_CONST)) {
        constants = parseConstantDeclarations();
    }

    if (check(TokenType::KW_TYPE)) {
        types = parseTypeDeclarations();
    }

    if (check(TokenType::KW_VAR)) {
        variables = parseVariableDeclarations();
    }

    return std::make_unique<DeclarationsBlock>(
        std::move(constants),
        std::move(types),
        std::move(variables)
    );
}

std::unique_ptr<ConstantDeclarations> Parser::parseConstantDeclarations() {
    expect(TokenType::KW_CONST, "Expected 'const'");

    std::vector<std::unique_ptr<ConstantDeclaration>> constants;

    if (match(TokenType::LBRACE)) {
        // Multiple declarations in block
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            constants.push_back(parseConstantDeclaration());
            expect(TokenType::SEMICOLON, "Expected ';' after constant declaration");
        }
        expect(TokenType::RBRACE, "Expected '}' after constant declarations");
    } else {
        // Single declaration
        constants.push_back(parseConstantDeclaration());
        expect(TokenType::SEMICOLON, "Expected ';' after constant declaration");
    }

    return std::make_unique<ConstantDeclarations>(std::move(constants));
}

std::unique_ptr<ConstantDeclaration> Parser::parseConstantDeclaration() {
    auto [name, isExported] = parseIdentifierDeclaration();
    expect(TokenType::OP_ASSIGN, "Expected '=' in constant declaration");
    auto value = parseExpression();
    return std::make_unique<ConstantDeclaration>(name, isExported, std::move(value));
}

std::unique_ptr<TypeDeclarations> Parser::parseTypeDeclarations() {
    expect(TokenType::KW_TYPE, "Expected 'type'");

    std::vector<std::unique_ptr<TypeDeclaration>> types;

    if (match(TokenType::LBRACE)) {
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            types.push_back(parseTypeDeclaration());
            expect(TokenType::SEMICOLON, "Expected ';' after type declaration");
        }
        expect(TokenType::RBRACE, "Expected '}' after type declarations");
    } else {
        types.push_back(parseTypeDeclaration());
        expect(TokenType::SEMICOLON, "Expected ';' after type declaration");
    }

    return std::make_unique<TypeDeclarations>(std::move(types));
}

std::unique_ptr<TypeDeclaration> Parser::parseTypeDeclaration() {
    auto [name, isExported] = parseIdentifierDeclaration();
    expect(TokenType::COLON, "Expected ':' in type declaration");
    auto type = parseType();
    return std::make_unique<TypeDeclaration>(name, isExported, std::move(type));
}

std::unique_ptr<VariableDeclarations> Parser::parseVariableDeclarations() {
    expect(TokenType::KW_VAR, "Expected 'var'");

    std::vector<std::unique_ptr<VariableDeclaration>> variables;

    if (match(TokenType::LBRACE)) {
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            auto vars = parseIdentifierListWithType();
            for (auto& v : vars) {
                variables.push_back(std::move(v));
            }
            expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
        }
        expect(TokenType::RBRACE, "Expected '}' after variable declarations");
    } else {
        auto vars = parseIdentifierListWithType();
        for (auto& v : vars) {
            variables.push_back(std::move(v));
        }
        expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    }

    return std::make_unique<VariableDeclarations>(std::move(variables));
}

std::unique_ptr<VariableDeclaration> Parser::parseVariableDeclaration() {
    auto [name, isExported] = parseIdentifierDeclaration();
    expect(TokenType::COLON, "Expected ':' in variable declaration");
    auto type = parseType();
    return std::make_unique<VariableDeclaration>(name, isExported, std::move(type));
}

std::vector<std::unique_ptr<VariableDeclaration>> Parser::parseIdentifierListWithType() {
    std::vector<std::pair<std::string, bool>> identifiers;

    do {
        identifiers.push_back(parseIdentifierDeclaration());
    } while (match(TokenType::COMMA));

    expect(TokenType::COLON, "Expected ':' after identifier list");
    auto type = std::shared_ptr(parseType());

    std::vector<std::unique_ptr<VariableDeclaration>> result;
    for (auto& identifier : identifiers) {
        result.push_back(std::make_unique<VariableDeclaration>(
            identifier.first,
            identifier.second,
            type
        ));
    }

    return result;
}

// ============================================================================
// Procedure Parsing
// ============================================================================

std::unique_ptr<ProcedureDeclaration> Parser::parseProcedureDeclaration() {
    expect(TokenType::KW_FN, "Expected 'fn'");

    auto [name, isExported] = parseIdentifierDeclaration();

    auto parameters = parseFormalParameters();

    expect(TokenType::OP_ARROW, "Expected '->' after parameters");
    auto returnType = parseReturnType();

    auto declarations = parseDeclarationSequence();

    // Parse body
    expect(TokenType::LBRACE, "Expected '{' to start procedure body");

    std::vector<std::unique_ptr<Statement>> statements;
    std::unique_ptr<Expression> returnExpression;

    while (!check(TokenType::RBRACE) && !check(TokenType::KW_RETURN) && !isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const ParseError&) {
            synchronize();
        }
    }

    if (match(TokenType::KW_RETURN)) {
        if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE)) {
            returnExpression = parseExpression();
        }
        match(TokenType::SEMICOLON);
    }

    expect(TokenType::RBRACE, "Expected '}' to end procedure body");

    auto body = std::make_unique<StatementsBlock>(std::move(statements));

    return std::make_unique<ProcedureDeclaration>(
        name,
        isExported,
        std::move(parameters),
        std::move(returnType),
        std::move(declarations),
        std::move(body),
        std::move(returnExpression)
    );
}

std::vector<std::unique_ptr<ProcedureParameter>> Parser::parseFormalParameters() {
    expect(TokenType::LPAREN, "Expected '(' for parameters");

    std::vector<std::unique_ptr<ProcedureParameter>> params;

    if (!check(TokenType::RPAREN)) {
        do {
            // Parse identifier list
            std::vector<std::string> names;
            do {
                Token nameToken = expect(TokenType::IDENTIFIER, "Expected parameter name");
                names.push_back(nameToken.lexeme);
            } while (match(TokenType::COMMA));

            expect(TokenType::COLON, "Expected ':' after parameter names");

            bool isReference = match(TokenType::AMPERSAND);
            auto type = std::shared_ptr(parseType());

            for (auto & name : names) {
                params.push_back(std::make_unique<ProcedureParameter>(
                    name, isReference, type
                ));
            }
        } while (match(TokenType::COMMA));
    }

    expect(TokenType::RPAREN, "Expected ')' after parameters");
    return params;
}

std::unique_ptr<Type> Parser::parseReturnType() {
    if (match(TokenType::KW_VOID)) {
        return std::make_unique<IdentifierType>("", "void");
    }
    return parseType();
}

// ============================================================================
// Type Parsing
// ============================================================================

std::unique_ptr<Type> Parser::parseType() {
    if (match(TokenType::OP_STAR)) {
        return parsePointerType();
    }

    if (check(TokenType::KW_STRUCT)) {
        return parseStructType();
    }

    if (check(TokenType::LPAREN)) {
        return parseProcedureType();
    }

    std::unique_ptr<Type> type = parseIdentifierType();

    // Check for array dimensions
    while (check(TokenType::LBRACKET)) {
        type = parseArrayType(std::move(type));
    }

    return type;
}

std::unique_ptr<IdentifierType> Parser::parseIdentifierType() {
    auto [moduleName, name] = parseQualifiedIdentifier();
    return std::make_unique<IdentifierType>(moduleName, name);
}

std::unique_ptr<ArrayType> Parser::parseArrayType(std::unique_ptr<Type> elementType) {
    expect(TokenType::LBRACKET, "Expected '[' for array type");

    if (match(TokenType::RBRACKET)) {
        // Open array type (no size specified)
        return std::make_unique<ArrayType>(std::move(elementType), nullptr);
    }

    auto length = parseExpression();
    expect(TokenType::RBRACKET, "Expected ']' after array length");

    auto arrayType = std::make_unique<ArrayType>(std::move(elementType), std::move(length));

    // Check for multi-dimensional arrays
    if (check(TokenType::LBRACKET)) {
        return parseArrayType(std::move(arrayType));
    }

    return arrayType;
}

std::unique_ptr<StructType> Parser::parseStructType() {
    expect(TokenType::KW_STRUCT, "Expected 'struct'");

    std::unique_ptr<IdentifierType> baseType;
    if (match(TokenType::LPAREN)) {
        baseType = parseIdentifierType();
        expect(TokenType::RPAREN, "Expected ')' after base type");
    }

    expect(TokenType::LBRACE, "Expected '{' for struct body");

    std::vector<std::unique_ptr<VariableDeclaration>> fields;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto fieldVars = parseIdentifierListWithType();
        for (auto& v : fieldVars) {
            fields.push_back(std::move(v));
        }
        expect(TokenType::SEMICOLON, "Expected ';' after field declaration");
    }

    expect(TokenType::RBRACE, "Expected '}' after struct body");

    return std::make_unique<StructType>(std::move(baseType), std::move(fields));
}

std::unique_ptr<PointerType> Parser::parsePointerType() {
    auto pointeeType = parseType();
    return std::make_unique<PointerType>(std::move(pointeeType));
}

std::unique_ptr<Type> Parser::parseProcedureType() {
    auto params = parseFormalParameters();
    expect(TokenType::OP_ARROW, "Expected '->' for procedure type");
    auto returnType = parseReturnType();
    return std::make_unique<ProcedureType>(std::move(params), std::move(returnType));
}

// ============================================================================
// Statement Parsing
// ============================================================================

std::unique_ptr<StatementsBlock> Parser::parseStatementSequence() {
    std::vector<std::unique_ptr<Statement>> statements;

    while (!check(TokenType::RBRACE) && !check(TokenType::KW_RETURN) &&
           !check(TokenType::KW_CASE) && !isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const ParseError&) {
            synchronize();
        }
    }

    return std::make_unique<StatementsBlock>(std::move(statements));
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (check(TokenType::KW_IF)) {
        return parseIfStatement();
    }
    if (check(TokenType::KW_WHILE)) {
        return parseWhileStatement();
    }
    if (check(TokenType::KW_DO)) {
        return parseDoWhileStatement();
    }
    if (check(TokenType::KW_FOR)) {
        return parseForStatement();
    }
    if (check(TokenType::KW_SWITCH)) {
        return parseSwitchStatement();
    }

    return parseAssignmentOrProcedureCall();
}

std::unique_ptr<Statement> Parser::parseAssignmentOrProcedureCall() {
    auto expr = parseDesignator();

    if (match(TokenType::OP_ASSIGN)) {
        auto value = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after assignment");
        return std::make_unique<AssignmentStatement>(std::move(expr), std::move(value));
    }

    if (check(TokenType::LPAREN)) {
        auto call = parseProcedureCall(std::move(expr));
        expect(TokenType::SEMICOLON, "Expected ';' after procedure call");
        return call;
    }

    expect(TokenType::SEMICOLON, "Expected ';' after expression");
    return nullptr;
}

std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    expect(TokenType::KW_IF, "Expected 'if'");
    auto condition = parseExpression();
    auto thenBranch = parseConditionBody();

    std::unique_ptr<Statement> elseBranch;
    if (match(TokenType::KW_ELSE)) {
        if (check(TokenType::KW_IF)) {
            elseBranch = parseIfStatement();
        } else {
            elseBranch = parseConditionBody();
        }
    }

    return std::make_unique<IfStatement>(
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch)
    );
}

std::unique_ptr<WhileStatement> Parser::parseWhileStatement() {
    expect(TokenType::KW_WHILE, "Expected 'while'");

    std::vector<std::unique_ptr<WhileBranch>> branches;

    auto condition = parseExpression();
    expect(TokenType::LBRACE, "Expected '{' after while condition");
    auto body = parseStatementSequence();
    expect(TokenType::RBRACE, "Expected '}' after while body");

    branches.push_back(std::make_unique<WhileBranch>(std::move(condition), std::move(body)));

    while (match(TokenType::KW_ELIF)) {
        auto elifCondition = parseExpression();
        expect(TokenType::LBRACE, "Expected '{' after elif condition");
        auto elifBody = parseStatementSequence();
        expect(TokenType::RBRACE, "Expected '}' after elif body");
        branches.push_back(std::make_unique<WhileBranch>(std::move(elifCondition), std::move(elifBody)));
    }

    return std::make_unique<WhileStatement>(std::move(branches));
}

std::unique_ptr<DoWhileStatement> Parser::parseDoWhileStatement() {
    expect(TokenType::KW_DO, "Expected 'do'");

    auto body = parseConditionBody();

    expect(TokenType::KW_WHILE, "Expected 'while' after do body");
    auto condition = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after do-while condition");

    return std::make_unique<DoWhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<ForStatement> Parser::parseForStatement() {
    expect(TokenType::KW_FOR, "Expected 'for'");
    expect(TokenType::LPAREN, "Expected '(' after 'for'");

    Token counterToken = expect(TokenType::IDENTIFIER, "Expected counter variable name");
    std::string counterName = counterToken.lexeme;

    expect(TokenType::OP_ASSIGN, "Expected '=' after counter name");
    auto rangeStart = parseExpression();

    expect(TokenType::COMMA, "Expected ',' after range start");
    auto rangeEnd = parseExpression();

    std::unique_ptr<Expression> step;
    if (match(TokenType::COMMA)) {
        step = parseExpression();
    }

    expect(TokenType::RPAREN, "Expected ')' after for header");

    auto body = parseConditionBody();

    return std::make_unique<ForStatement>(
        counterName,
        std::move(rangeStart),
        std::move(rangeEnd),
        std::move(step),
        std::move(body)
    );
}

std::unique_ptr<SwitchStatement> Parser::parseSwitchStatement() {
    expect(TokenType::KW_SWITCH, "Expected 'switch'");
    auto selector = parseExpression();
    expect(TokenType::LBRACE, "Expected '{' after switch expression");

    std::vector<std::unique_ptr<SwitchCase>> cases;
    while (check(TokenType::KW_CASE)) {
        cases.push_back(parseSwitchCase());
    }

    expect(TokenType::RBRACE, "Expected '}' after switch cases");

    return std::make_unique<SwitchStatement>(std::move(selector), std::move(cases));
}

std::unique_ptr<SwitchCase> Parser::parseSwitchCase() {
    expect(TokenType::KW_CASE, "Expected 'case'");

    std::vector<std::unique_ptr<CaseLabel>> labels;
    do {
        labels.push_back(parseCaseLabel());
    } while (match(TokenType::COMMA));

    expect(TokenType::COLON, "Expected ':' after case labels");

    auto body = parseStatementSequence();

    return std::make_unique<SwitchCase>(std::move(labels), std::move(body));
}

std::unique_ptr<CaseLabel> Parser::parseCaseLabel() {
    auto value = parseExpression();

    std::unique_ptr<Expression> endValue;
    if (match(TokenType::OP_DOTDOT)) {
        endValue = parseExpression();
    }

    return std::make_unique<CaseLabel>(std::move(value), std::move(endValue));
}

std::unique_ptr<Statement> Parser::parseConditionBody() {
    if (match(TokenType::LBRACE)) {
        auto block = parseStatementSequence();
        expect(TokenType::RBRACE, "Expected '}' after block");
        return block;
    }
    return parseStatement();
}

// ============================================================================
// Expression Parsing
// ============================================================================

std::unique_ptr<Expression> Parser::parseExpression() {
    auto left = parseSimpleExpression();

    if (check(TokenType::OP_EQ) || check(TokenType::OP_NE) ||
        check(TokenType::OP_LT) || check(TokenType::OP_LE) ||
        check(TokenType::OP_GT) || check(TokenType::OP_GE) ||
        check(TokenType::KW_IS)) {

        Token opToken = advance();
        BinaryExpression::Op op;

        switch (opToken.type) {
            case TokenType::OP_EQ:  op = BinaryExpression::Op::Eq; break;
            case TokenType::OP_NE:  op = BinaryExpression::Op::Neq; break;
            case TokenType::OP_LT:  op = BinaryExpression::Op::Lt; break;
            case TokenType::OP_LE:  op = BinaryExpression::Op::Lte; break;
            case TokenType::OP_GT:  op = BinaryExpression::Op::Gt; break;
            case TokenType::OP_GE:  op = BinaryExpression::Op::Gte; break;
            case TokenType::KW_IS:  op = BinaryExpression::Op::Is; break;
            default: throw error("Unexpected relational operator");
        }

        BinaryExpression::RightType right;
        if (op == BinaryExpression::Op::Is) {
            right = parseIdentifierType();
        } else {
            right = parseSimpleExpression();
        }
        left = std::make_unique<BinaryExpression>(std::move(left), std::move(right), op);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parseSimpleExpression() {
    // Handle optional leading sign
    UnaryExpression::Op unaryOp;
    bool hasUnary = false;

    if (match(TokenType::OP_PLUS)) {
        hasUnary = true;
        unaryOp = UnaryExpression::Op::Plus;
    } else if (match(TokenType::OP_MINUS)) {
        hasUnary = true;
        unaryOp = UnaryExpression::Op::Negate;
    }

    auto left = parseTerm();

    if (hasUnary) {
        left = std::make_unique<UnaryExpression>(std::move(left), unaryOp);
    }

    while (check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS) ||
           check(TokenType::OP_OR)) {

        Token opToken = advance();
        BinaryExpression::Op op;

        switch (opToken.type) {
            case TokenType::OP_PLUS:  op = BinaryExpression::Op::Add; break;
            case TokenType::OP_MINUS: op = BinaryExpression::Op::Sub; break;
            case TokenType::OP_OR:    op = BinaryExpression::Op::Or; break;
            default: throw error("Unexpected additive operator");
        }

        auto right = parseTerm();
        left = std::make_unique<BinaryExpression>(std::move(left), std::move(right), op);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parseTerm() {
    auto left = parseFactor();

    while (check(TokenType::OP_STAR) || check(TokenType::OP_SLASH) ||
           check(TokenType::KW_DIV) || check(TokenType::KW_MOD) ||
           check(TokenType::OP_AND)) {

        Token opToken = advance();
        BinaryExpression::Op op;

        switch (opToken.type) {
            case TokenType::OP_STAR:  op = BinaryExpression::Op::Mul; break;
            case TokenType::OP_SLASH: op = BinaryExpression::Op::FDiv; break;
            case TokenType::KW_DIV:   op = BinaryExpression::Op::IDiv; break;
            case TokenType::KW_MOD:   op = BinaryExpression::Op::Mod; break;
            case TokenType::OP_AND:   op = BinaryExpression::Op::And; break;
            default: throw error("Unexpected multiplicative operator");
        }

        auto right = parseFactor();
        left = std::make_unique<BinaryExpression>(std::move(left), std::move(right), op);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parseFactor() {
    // Literals
    if (match(TokenType::INTEGER)) {
        const std::string& lexeme = previous().lexeme;
        int64_t value;
        if (lexeme.size() > 2 && lexeme[0] == '0' && lexeme[1] == 'x') {
            value = std::strtoll(lexeme.c_str() + 2, nullptr, 16);
        } else {
            value = std::strtoll(lexeme.c_str(), nullptr, 10);
        }
        return std::make_unique<IntegerLiteral>(value);
    }

    if (match(TokenType::FLOAT)) {
        double value = std::strtod(previous().lexeme.c_str(), nullptr);
        return std::make_unique<RealLiteral>(value);
    }

    if (match(TokenType::STRING)) {
        std::string value = previous().lexeme;
        if (value.size() >= 2) {
            value = value.substr(1, value.size() - 2);
        }
        return std::make_unique<StringLiteral>(value);
    }

    if (match(TokenType::KW_TRUE)) {
        return std::make_unique<BooleanLiteral>(true);
    }

    if (match(TokenType::KW_FALSE)) {
        return std::make_unique<BooleanLiteral>(false);
    }

    if (match(TokenType::KW_NIL)) {
        return std::make_unique<Nil>();
    }

    // Parenthesized expression
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    // Unary not
    if (match(TokenType::OP_NOT)) {
        auto operand = parseFactor();
        return std::make_unique<UnaryExpression>(std::move(operand), UnaryExpression::Op::Not);
    }

    auto designator = parseDesignator();

    if (check(TokenType::LPAREN)) {
        return parseProcedureCall(std::move(designator));
    }

    return designator;
}

std::unique_ptr<Expression> Parser::parseDesignator() {
    // Check for dereference prefix
    bool isDereference = match(TokenType::OP_STAR);

    auto [moduleName, name] = parseQualifiedIdentifier();
    std::unique_ptr<Expression> expr = std::make_unique<IdentifierExpression>(moduleName, name);

    if (isDereference) {
        expr = std::make_unique<DereferenceExpression>(std::move(expr));
    }

    // Parse selectors
    while (true) {
        if (match(TokenType::DOT)) {
            Token memberToken = expect(TokenType::IDENTIFIER, "Expected member name after '.'");
            expr = std::make_unique<MemberAccessExpression>(std::move(expr), memberToken.lexeme);
        } else if (match(TokenType::LBRACKET)) {
            auto index = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']' after array index");
            expr = std::make_unique<ArrayAccessExpression>(std::move(expr), std::move(index));
        } else if (check(TokenType::LPAREN)) {
            // Could be a type guard: expr(Type) or expr(Module.Type)
            // Look ahead to determine if this is a type guard
            // Type guard pattern: ( Identifier ) or ( Identifier . Identifier )

            size_t saved = current_;
            advance(); // consume '('

            if (check(TokenType::IDENTIFIER)) {
                advance(); // consume first identifier

                // Check for qualified identifier: Module.Type
                if (check(TokenType::DOT)) {
                    advance(); // consume '.'
                    if (check(TokenType::IDENTIFIER)) {
                        advance(); // consume second identifier
                    }
                }

                if (check(TokenType::RPAREN)) {
                    // This is a type guard: expr(Type) or expr(Module.Type)
                    advance(); // consume ')'

                    // Get the type name from what we parsed
                    // Reconstruct the qualified identifier
                    size_t typeStart = saved + 1; // after '('
                    size_t typeEnd = current_ - 1; // before ')'

                    std::string moduleName;
                    std::string typeName;

                    // Re-parse the type identifier properly
                    current_ = saved;
                    advance(); // consume '('
                    Token first = advance(); // get first identifier

                    if (check(TokenType::DOT)) {
                        advance(); // consume '.'
                        Token second = advance(); // get second identifier
                        moduleName = first.lexeme;
                        typeName = second.lexeme;
                    } else {
                        typeName = first.lexeme;
                    }

                    advance(); // consume ')'

                    // Create a type guard using ProcedureCall with isTypeGuard flag
                    auto typeExpr = std::make_unique<IdentifierExpression>(moduleName, typeName);
                    std::vector<std::unique_ptr<Expression>> args;
                    args.push_back(std::move(typeExpr));

                    auto typeGuard = std::make_unique<ProcedureCall>(std::move(expr), std::move(args));
                    typeGuard->isTypeGuard = true;
                    expr = std::move(typeGuard);

                    continue; // Continue parsing more selectors
                }
            }

            // Not a type guard, restore position and break
            current_ = saved;
            break;
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<ProcedureCall> Parser::parseProcedureCall(std::unique_ptr<Expression> callee) {
    expect(TokenType::LPAREN, "Expected '(' for procedure call");

    std::vector<std::unique_ptr<Expression>> args;

    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }

    expect(TokenType::RPAREN, "Expected ')' after arguments");

    return std::make_unique<ProcedureCall>(std::move(callee), std::move(args));
}

// ============================================================================
// Helper Methods
// ============================================================================

std::pair<std::string, bool> Parser::parseIdentifierDeclaration() {
    bool isExported = match(TokenType::KW_EXPORT);
    Token nameToken = expect(TokenType::IDENTIFIER, "Expected identifier");
    return {nameToken.lexeme, isExported};
}

std::pair<std::string, std::string> Parser::parseQualifiedIdentifier() {
    Token first = expect(TokenType::IDENTIFIER, "Expected identifier");

    if (match(TokenType::DOT)) {
        Token second = expect(TokenType::IDENTIFIER, "Expected identifier after '.'");
        return {first.lexeme, second.lexeme};  // moduleName, name
    }

    return {"", first.lexeme};  // no module, just name
}

} // namespace obould
