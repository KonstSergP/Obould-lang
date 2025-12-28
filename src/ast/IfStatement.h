#pragma once
#include <memory>
#include "Expression.h"
#include "Statement.h"


class IfStatement : public Statement
{
public:
    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> then_branch,
                std::unique_ptr<Statement> else_branch)
        : condition(std::move(condition)),
          thenBranch(std::move(then_branch)),
          elseBranch(std::move(else_branch)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
};
