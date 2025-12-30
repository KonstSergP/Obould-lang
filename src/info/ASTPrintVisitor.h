#pragma once
#include <iostream>
#include <string>
#include "ast/ASTVisitor.h"


class ASTPrintVisitor : public ASTVisitor
{
public:
    explicit ASTPrintVisitor(std::ostream& os = std::cout);

    // Expressions
    void visit(IntegerLiteral& node) override;
    void visit(RealLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(Nil& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(ArrayAccessExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(DereferenceExpression& node) override;

    // Statements
    void visit(ProcedureCall& node) override;
    void visit(StatementsBlock& node) override;
    void visit(AssignmentStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(WhileBranch& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(SwitchCase& node) override;
    void visit(CaseLabel& node) override;


    // Types
    void visit(IdentifierType& node) override;
    void visit(ArrayType& node) override;
    void visit(OpenArrayType& node) override;
    void visit(StructType& node) override;
    void visit(PointerType& node) override;
    void visit(ProcedureType& node) override;

    // Declarations
    void visit(ConstantDeclaration& node) override;
    void visit(TypeDeclaration& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ProcedureDeclaration& node) override;
    void visit(ProcedureParameter& node) override;
    void visit(ConstantDeclarations& node) override;
    void visit(TypeDeclarations& node) override;
    void visit(VariableDeclarations& node) override;
    void visit(DeclarationsBlock& node) override;
    void visit(Import& node) override;
    void visit(Module& node) override;

private:
    std::ostream& os;
    int indentLevel = 0;

    void printIndent() const;
    static std::string binOpToString(BinaryExpression::Op op);
    static std::string unOpToString(UnaryExpression::Op op);
};
