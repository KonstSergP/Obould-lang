#pragma once
#include <memory>
#include "Expression.h"


class CaseLabel : public ASTNode
{
public:
    CaseLabel(std::unique_ptr<Expression> value, std::unique_ptr<Expression> end_value)
        : value(std::move(value)),
          endValue(std::move(end_value)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> value;
    std::unique_ptr<Expression> endValue;
};
