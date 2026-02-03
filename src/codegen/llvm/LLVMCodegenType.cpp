#include "LLVMCodegen.h"


namespace obould
{
void LLVMCodegenVisitor::visit(IdentifierType& node) {}
void LLVMCodegenVisitor::visit(ArrayType& node) {}
void LLVMCodegenVisitor::visit(OpenArrayType& node) {}
void LLVMCodegenVisitor::visit(PointerType& node) {}
void LLVMCodegenVisitor::visit(ProcedureType& node) {}

void LLVMCodegenVisitor::visit(StructType& node)
{
    auto type = node.resolvedType;
    if (descriptors.find(type.get()) == descriptors.end()) {
        descriptors[type.get()] = createStructDescriptor(type);
    }
}
}
