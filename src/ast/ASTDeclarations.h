#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "ASTCore.h"
#include "ASTStatements.h"

namespace obould
{
class Module;

class ConstantDeclaration : public ASTNode
{
public:
    ConstantDeclaration(std::string name, bool isExported, std::unique_ptr<Expression> value)
        : name(std::move(name)),
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
    TypeDeclaration(std::string name, bool isExported, std::unique_ptr<Type> type)
        : name(std::move(name)),
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
    VariableDeclaration(std::string name, bool isExported, std::shared_ptr<Type> type)
        : name(std::move(name)),
          isExported(isExported),
          type(std::move(type)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::shared_ptr<Type> type;
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


class ProcedureParameter : public ASTNode
{
public:
    ProcedureParameter(std::string name, bool isReference, std::shared_ptr<Type> type)
        : name(std::move(name)),
          isReference(isReference),
          type(std::move(type)) {}

    void accept(ASTVisitor& v) override;

    std::string name;
    bool isReference;
    std::shared_ptr<Type> type;
};


class ProcedureDeclaration : public ASTNode
{
public:
    ProcedureDeclaration(std::string name, bool isExported,
                         std::vector<std::unique_ptr<ProcedureParameter>> parameters,
                         std::unique_ptr<Type> returnType,
                         std::unique_ptr<DeclarationsBlock> declarations, std::unique_ptr<StatementsBlock> body,
                         std::unique_ptr<Expression> returnExpression)
        : name(std::move(name)),
          isExported(isExported),
          parameters(std::move(parameters)),
          returnType(std::move(returnType)),
          declarations(std::move(declarations)),
          body(std::move(body)),
          returnExpression(std::move(returnExpression)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::vector<std::unique_ptr<ProcedureParameter>> parameters;
    std::unique_ptr<Type> returnType;
    std::unique_ptr<DeclarationsBlock> declarations;
    std::unique_ptr<StatementsBlock> body;
    std::unique_ptr<Expression> returnExpression;
    std::shared_ptr<TypeInfo> resolvedType;
};

class Import : public ASTNode
{
public:
    Import(std::string localName, std::string realName)
        : localName(std::move(localName)),
          realName(std::move(realName)) {}

    void accept(ASTVisitor& v) override;


    std::string localName;
    std::string realName;
    std::unique_ptr<Module> module;
};

struct Properties
{
    bool needsGC = false;
    bool needsArgs = false;
};

class Module : public ASTNode
{
public:
    Module(std::string name,
           std::vector<std::unique_ptr<Import>> imports,
           std::unique_ptr<DeclarationsBlock> declarations,
           std::vector<std::unique_ptr<ProcedureDeclaration>> procedures,
           Properties properties = {})
        : name(std::move(name)),
          imports(std::move(imports)),
          declarations(std::move(declarations)),
          procedures(std::move(procedures)),
          properties(std::move(properties)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    Properties properties;
    std::vector<std::unique_ptr<Import>> imports;
    std::unique_ptr<DeclarationsBlock> declarations;
    std::vector<std::unique_ptr<ProcedureDeclaration>> procedures;
};
}
