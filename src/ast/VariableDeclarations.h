#pragma once
#include <memory>
#include <vector>
#include "ASTNode.h"
#include "VariableDeclaration.h"


class VariableDeclarations: public ASTNode
{
public:
    explicit VariableDeclarations(std::vector<std::unique_ptr<VariableDeclaration>> variables)
        : variables(std::move(variables)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<VariableDeclaration>> variables;
};
