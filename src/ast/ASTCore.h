#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>


class ASTVisitor;
class TypeInfo;

using ConstValue = std::variant<int64_t, double, bool, std::string>;


class ASTNode
{
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

class Expression : public virtual ASTNode
{
public:
    std::shared_ptr<TypeInfo> resolvedType;
    std::optional<ConstValue> constantValue;
    bool isLvalue;
};

class Statement : public virtual ASTNode {};

class Type : public virtual ASTNode
{
public:
    std::shared_ptr<TypeInfo> resolvedType;
};
