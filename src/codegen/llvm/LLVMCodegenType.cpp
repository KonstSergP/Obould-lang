#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


namespace obould
{
void LLVMCodegenVisitor::visit(IdentifierType& node) {}
void LLVMCodegenVisitor::visit(OpenArrayType& node) {}
void LLVMCodegenVisitor::visit(ProcedureType& node) {}
void LLVMCodegenVisitor::visit(IncompleteType& node) {}

void LLVMCodegenVisitor::visit(PointerType& node) { node.type->accept(*this); }

void LLVMCodegenVisitor::visit(ArrayType& node)
{
    if (lengths.find(node.resolvedType.get()) == lengths.end()) {
        lengths[node.resolvedType.get()] = builder->getInt64(node.resolvedType->length);
    }
    node.elementType->accept(*this);
}

void LLVMCodegenVisitor::visit(StructType& node)
{
    auto& type = node.resolvedType;
    if (descriptors.find(type.get()) == descriptors.end()) {
        descriptors[type.get()] = createStructDescriptor(type);
    }

    for (const auto& fieldDecl : node.fields) {
        fieldDecl->type->accept(*this);
    }
}
}
