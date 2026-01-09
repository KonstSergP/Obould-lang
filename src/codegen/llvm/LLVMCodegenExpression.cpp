#include "LLVMCodegen.h"


void LLVMCodegenVisitor::visit(IntegerLiteral& node) {}
void LLVMCodegenVisitor::visit(RealLiteral& node) {}
void LLVMCodegenVisitor::visit(BooleanLiteral& node) {}
void LLVMCodegenVisitor::visit(StringLiteral& node) {}
void LLVMCodegenVisitor::visit(Nil& node) {}
void LLVMCodegenVisitor::visit(BinaryExpression& node) {}
void LLVMCodegenVisitor::visit(UnaryExpression& node) {}
void LLVMCodegenVisitor::visit(IdentifierExpression& node) {}
void LLVMCodegenVisitor::visit(ArrayAccessExpression& node) {}
void LLVMCodegenVisitor::visit(MemberAccessExpression& node) {}
void LLVMCodegenVisitor::visit(DereferenceExpression& node) {}
