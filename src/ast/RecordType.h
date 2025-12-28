#pragma once
#include <memory>
#include <vector>
#include "IdentifierType.h"
#include "VariableDeclaration.h"


class RecordType : public Type
{
public:
    RecordType(const std::string& name, std::unique_ptr<IdentifierType> baseType,
               std::vector<std::unique_ptr<VariableDeclaration>> fields)
        : name(name),
          baseType(std::move(baseType)),
          fields(std::move(fields)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    std::unique_ptr<IdentifierType> baseType;
    std::vector<std::unique_ptr<VariableDeclaration>> fields;
};
