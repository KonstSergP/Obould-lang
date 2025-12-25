#pragma once


class ASTVisitor;


class ASTNode
{
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};
