#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


namespace obould
{
void LLVMCodegenVisitor::visit(IdentifierType& node) {}

void LLVMCodegenVisitor::visit(ArrayType& node)
{
    auto& type = node.resolvedType;
    while (type->kind == TypeKind::Array) {
        if (lengths.find(type.get()) == lengths.end()) {
            lengths[type.get()] = builder->getInt64(std::get<int64_t>(*node.length->constantValue));
        }
        type = type->baseType;
    }
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
