#pragma once


class ASTVisitor;


class ASTNode
{
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

class Expression : public virtual ASTNode {};

class Statement : public virtual ASTNode {};

class Type : public virtual ASTNode {};
