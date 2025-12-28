#pragma once
#include <memory>
#include "Expression.h"
#include "StatementsBlock.h"


class WhileBranch : public ASTNode
{
public:
    WhileBranch(std::unique_ptr<Expression> condition, std::unique_ptr<StatementsBlock> body)
        : condition(std::move(condition)),
          body(std::move(body)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> condition;
    std::unique_ptr<StatementsBlock> body;
};
