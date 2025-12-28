#pragma once
#include <memory>
#include "Type.h"


class PointerType : public Type
{
public:
    explicit PointerType(std::unique_ptr<Type> type) : type(std::move(type)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Type> type;
};
