#pragma once
#include <memory>
#include <string>

#include "Expression.h"


class MemberAccessExpression : public Expression
{
public:
    MemberAccessExpression(std::unique_ptr<Expression> obj, std::string field)
        : object(std::move(obj)), memberName(std::move(field)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> object;
    std::string memberName;
};
