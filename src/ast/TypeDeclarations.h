#pragma once
#include <memory>
#include <vector>
#include "ASTNode.h"
#include "TypeDeclaration.h"


class TypeDeclarations: public ASTNode
{
public:
    explicit TypeDeclarations(std::vector<std::unique_ptr<TypeDeclaration>> types)
        : types(std::move(types)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<TypeDeclaration>> types;
};
