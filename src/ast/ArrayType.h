#pragma once
#include <memory>
#include "Type.h"


class ArrayType : public Type
{
public:
    ArrayType(std::unique_ptr<Type> elementType, int length)
        : elementType(std::move(elementType)),
          length(length) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Type> elementType;
    int length;
};
