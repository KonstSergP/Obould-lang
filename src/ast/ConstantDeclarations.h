#pragma once
#include <memory>
#include <vector>
#include "ASTNode.h"
#include "ConstantDeclaration.h"


class ConstantDeclarations: public ASTNode
{
public:
    explicit ConstantDeclarations(std::vector<std::unique_ptr<ConstantDeclaration>> constants)
        : constants(std::move(constants)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<ConstantDeclaration>> constants;
};
