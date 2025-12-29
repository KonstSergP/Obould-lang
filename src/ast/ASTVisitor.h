#pragma once
#include "ASTCore.h"
#include "ASTDeclarations.h"
#include "ASTExpressions.h"
#include "ASTStatements.h"
#include "ASTTypes.h"


class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Expressions
    virtual void visit(IntegerLiteral& node) = 0;
    virtual void visit(RealLiteral& node) = 0;
    virtual void visit(BooleanLiteral& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(Nil& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(IdentifierExpression& node) = 0;
    virtual void visit(ArrayAccessExpression& node) = 0;
    virtual void visit(MemberAccessExpression& node) = 0;
    virtual void visit(DereferenceExpression& node) = 0;

    // Statements
    virtual void visit(ProcedureCall& node) = 0;
    virtual void visit(AssignmentStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(WhileBranch& node) = 0;
    virtual void visit(DoWhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(SwitchStatement& node) = 0;
    virtual void visit(SwitchCase& node) = 0;
    virtual void visit(CaseLabel& node) = 0;
    virtual void visit(StatementsBlock& node) = 0;

    // Types
    virtual void visit(IdentifierType& node) = 0;
    virtual void visit(ArrayType& node) = 0;
    virtual void visit(OpenArrayType& node) = 0;
    virtual void visit(StructType& node) = 0;
    virtual void visit(PointerType& node) = 0;
    virtual void visit(ProcedureType& node) = 0;

    // Declarations
    virtual void visit(ConstantDeclaration& node) = 0;
    virtual void visit(TypeDeclaration& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(ProcedureDeclaration& node) = 0;
    virtual void visit(ProcedureParameter& node) = 0;
    virtual void visit(ConstantDeclarations& node) = 0;
    virtual void visit(TypeDeclarations& node) = 0;
    virtual void visit(VariableDeclarations& node) = 0;
    virtual void visit(DeclarationsBlock& node) = 0;
    virtual void visit(Import& node) = 0;
    virtual void visit(Module& node) = 0;
};
