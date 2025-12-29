#pragma once
#include <memory>
#include <string>
#include <vector>
#include "ASTCore.h"
#include "ASTStatements.h"
#include "ProcedureParameter.h"


class Import : public ASTNode
{
public:
    Import(const std::string& localName, const std::string& realName)
        : localName(localName),
          realName(realName) {}

    void accept(ASTVisitor& v) override;


    std::string localName;
    std::string realName;
};


class ConstantDeclaration : public ASTNode
{
public:
    ConstantDeclaration(const std::string& name, bool isExported, std::unique_ptr<Expression> value)
        : name(name),
          isExported(isExported),
          value(std::move(value)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::unique_ptr<Expression> value;
};


class ConstantDeclarations : public ASTNode
{
public:
    explicit ConstantDeclarations(std::vector<std::unique_ptr<ConstantDeclaration>> constants)
        : constants(std::move(constants)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<ConstantDeclaration>> constants;
};

class TypeDeclaration : public ASTNode
{
public:
    TypeDeclaration(const std::string& name, bool isExported, std::unique_ptr<Type> type)
        : name(name),
          isExported(isExported),
          type(std::move(type)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::unique_ptr<Type> type;
};


class TypeDeclarations : public ASTNode
{
public:
    explicit TypeDeclarations(std::vector<std::unique_ptr<TypeDeclaration>> types)
        : types(std::move(types)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<TypeDeclaration>> types;
};


class VariableDeclaration : public ASTNode
{
public:
    VariableDeclaration(const std::string& name, bool isExported, std::unique_ptr<Type> type)
        : name(name),
          isExported(isExported),
          type(std::move(type)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::unique_ptr<Type> type;
};


class VariableDeclarations : public ASTNode
{
public:
    explicit VariableDeclarations(std::vector<std::unique_ptr<VariableDeclaration>> variables)
        : variables(std::move(variables)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<VariableDeclaration>> variables;
};


class DeclarationsBlock : public ASTNode
{
public:
    DeclarationsBlock(std::unique_ptr<ConstantDeclarations> constants,
                      std::unique_ptr<TypeDeclarations> types, std::unique_ptr<VariableDeclarations> variables)
        : constants(std::move(constants)),
          types(std::move(types)),
          variables(std::move(variables)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<ConstantDeclarations> constants;
    std::unique_ptr<TypeDeclarations> types;
    std::unique_ptr<VariableDeclarations> variables;
};


class ProcedureDeclaration : public ASTNode
{
public:
    ProcedureDeclaration(const std::string& name, bool isExported,
                         std::vector<std::unique_ptr<ProcedureParameter>> parameters, std::unique_ptr<Type> returnType,
                         std::unique_ptr<StatementsBlock> body, std::unique_ptr<Expression> returnExpression)
        : name(name),
          isExported(isExported),
          parameters(std::move(parameters)),
          returnType(std::move(returnType)),
          body(std::move(body)),
          returnExpression(std::move(returnExpression)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::vector<std::unique_ptr<ProcedureParameter>> parameters;
    std::unique_ptr<Type> returnType;
    std::unique_ptr<StatementsBlock> body;
    std::unique_ptr<Expression> returnExpression;
};


class Module : public ASTNode
{
public:
    Module(const std::string& name,
              std::vector<std::unique_ptr<Import>> imports,
              std::unique_ptr<DeclarationsBlock> declarations,
              std::vector<std::unique_ptr<ProcedureDeclaration>> procedures)
        : name(name),
          imports(std::move(imports)),
          declarations(std::move(declarations)),
          procedures(std::move(procedures)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    std::vector<std::unique_ptr<Import>> imports;
    std::unique_ptr<DeclarationsBlock> declarations;
    std::vector<std::unique_ptr<ProcedureDeclaration>> procedures;
};
