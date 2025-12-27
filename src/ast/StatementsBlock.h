#pragma once
#include <memory>
#include <vector>
#include "ASTNode.h"
#include "Statement.h"


class StatementsBlock : public ASTNode
{
public:
    explicit StatementsBlock(std::vector<std::unique_ptr<Statement>> statements)
        : statements(std::move(statements)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<Statement>> statements;
};
