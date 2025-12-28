#pragma once
#include <memory>
#include "Expression.h"
#include "Statement.h"


class AssignmentStatement : public Statement
{
public:
    AssignmentStatement(std::unique_ptr<Expression> target, std::unique_ptr<Expression> value)
        : target(std::move(target)),
          value(std::move(value)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> value;
};
