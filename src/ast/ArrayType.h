#pragma once
#include <memory>

#include "Expression.h"
#include "Type.h"


class ArrayType : public Type
{
public:
    ArrayType(std::unique_ptr<Type> elementType, std::unique_ptr<Expression> length)
        : elementType(std::move(elementType)),
          length(std::move(length)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Type> elementType;
    std::unique_ptr<Expression> length;
};
