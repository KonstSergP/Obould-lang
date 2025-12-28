#pragma once
#include <memory>
#include "Type.h"


class OpenArrayType : public Type
{
public:
    explicit OpenArrayType(std::unique_ptr<Type> elementType) : elementType(std::move(elementType)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Type> elementType;
};
