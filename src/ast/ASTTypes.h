#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "ASTCore.h"
#include "ASTDeclarations.h"


class IdentifierType : public Type
{
public:
    IdentifierType(std::string  moduleName, std::string  name)
        : name(std::move(name)),
          moduleName(std::move(moduleName)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    std::string moduleName;
};


class ArrayType : public Type
{
public:
    ArrayType(std::unique_ptr<Type> elementType, std::unique_ptr<Expression> length)
        : elementType(std::move(elementType)),
          length(std::move(length)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Type> elementType;
    std::unique_ptr<Expression> length;
};


class OpenArrayType : public Type
{
public:
    explicit OpenArrayType(std::unique_ptr<Type> elementType) : elementType(std::move(elementType)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Type> elementType;
};


class PointerType : public Type
{
public:
    explicit PointerType(std::unique_ptr<Type> type) : type(std::move(type)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Type> type;
};


class ProcedureType : public Type
{
public:
    ProcedureType(std::vector<std::unique_ptr<ProcedureParameter>> parameters, std::unique_ptr<Type> returnType)
        : parameters(std::move(parameters)),
          returnType(std::move(returnType)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<ProcedureParameter>> parameters;
    std::unique_ptr<Type> returnType;
};


class StructType : public Type
{
public:
    StructType(const std::string& name, std::unique_ptr<IdentifierType> baseType,
               std::vector<std::unique_ptr<VariableDeclaration>> fields)
        : name(name),
          baseType(std::move(baseType)),
          fields(std::move(fields)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    std::unique_ptr<IdentifierType> baseType;
    std::vector<std::unique_ptr<VariableDeclaration>> fields;
};
