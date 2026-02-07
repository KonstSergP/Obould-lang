#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


namespace obould
{
void LLVMCodegenVisitor::visit(IdentifierType& node) {}

void LLVMCodegenVisitor::visit(ArrayType& node)
{
    if (lengths.find(node.resolvedType.get()) == lengths.end()) {
        lengths[node.resolvedType.get()] = builder->getInt64(std::get<int64_t>(*node.length->constantValue));
    }
    node.elementType->accept(*this);
}

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
