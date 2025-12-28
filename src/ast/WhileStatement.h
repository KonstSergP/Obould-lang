#pragma once
#include <memory>
#include <vector>
#include "Statement.h"
#include "WhileBranch.h"


class WhileStatement : public Statement
{
public:
    explicit WhileStatement(std::vector<std::unique_ptr<WhileBranch>> branches) : branches(std::move(branches)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<WhileBranch>> branches;
};
