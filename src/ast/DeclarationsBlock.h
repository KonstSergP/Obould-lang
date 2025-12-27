#pragma once
#include <memory>
#include "ASTNode.h"
#include "ConstantDeclarations.h"
#include "TypeDeclarations.h"
#include "VariableDeclarations.h"


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
