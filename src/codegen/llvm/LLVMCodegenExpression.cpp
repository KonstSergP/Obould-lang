#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


void LLVMCodegenVisitor::visit(IntegerLiteral& node)
{
    lastValue = getConstantValue(node);
}

void LLVMCodegenVisitor::visit(RealLiteral& node)
{
    lastValue = getConstantValue(node);
}

void LLVMCodegenVisitor::visit(BooleanLiteral& node)
{
    lastValue = getConstantValue(node);
}

void LLVMCodegenVisitor::visit(StringLiteral& node)
{
    lastValue = getConstantValue(node);
}

void LLVMCodegenVisitor::visit(Nil& node)
{
    auto* ty = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    lastValue = llvm::ConstantPointerNull::get(ty);
}

void LLVMCodegenVisitor::visit(BinaryExpression& node)
{
    if (node.constantValue.has_value()) {
        lastValue = getConstantValue(node);
        return;
    }
    node.left->accept(*this);
    llvm::Value* lhs = lastValue;

    llvm::Value* rhs = nullptr;
    if (std::holds_alternative<std::unique_ptr<Expression>>(node.right)) {
        std::get<std::unique_ptr<Expression>>(node.right)->accept(*this);
        rhs = lastValue;
    }
    else {
        // TODO: реализовать для IS
        lastValue = nullptr;
        return;
    }

    if (!lhs || !rhs) {
        lastValue = nullptr;
        return;
    }

    auto resultType = node.resolvedType;
    if (!resultType) {
        lastValue = nullptr;
        return;
    }

    bool isInt = lhs->getType()->isIntegerTy();
    bool isReal = lhs->getType()->isFloatingPointTy();
    using Op = BinaryExpression::Op;

    switch (node.op) {
    case Op::Add:
        lastValue = isInt
                        ? builder->CreateAdd(lhs, rhs, "add")
                        : builder->CreateFAdd(lhs, rhs, "fadd");
        break;
    case Op::Sub:
        lastValue = isInt
                        ? builder->CreateSub(lhs, rhs, "sub")
                        : builder->CreateFSub(lhs, rhs, "fsub");
        break;
    case Op::Mul:
        lastValue = isInt
                        ? builder->CreateMul(lhs, rhs, "mul")
                        : builder->CreateFMul(lhs, rhs, "fmul");
        break;
    case Op::FDiv:
        lastValue = builder->CreateFDiv(lhs, rhs, "fdiv");
        break;
    case Op::IDiv:
    {
        llvm::Value* q = builder->CreateSDiv(lhs, rhs, "q_trunc");
        llvm::Value* r = builder->CreateSRem(lhs, rhs, "r_trunc");

        llvm::Value* zero = llvm::ConstantInt::get(lhs->getType(), 0);
        llvm::Value* isNegRem = builder->CreateICmpSLT(r, zero, "rem_is_neg");

        llvm::Value* minusOne = llvm::ConstantInt::get(lhs->getType(), -1);
        llvm::Value* adjustment = builder->CreateSelect(isNegRem, minusOne, zero);
        llvm::Value* finalDiv = builder->CreateAdd(q, adjustment, "div_oberon");

        lastValue = finalDiv;
        break;
    }
    case Op::Mod:
    {
        llvm::Value* r = builder->CreateSRem(lhs, rhs, "r_trunc");

        llvm::Value* zero = llvm::ConstantInt::get(lhs->getType(), 0);
        llvm::Value* isNegRem = builder->CreateICmpSLT(r, zero, "rem_is_neg");

        llvm::Value* adjustment = builder->CreateSelect(isNegRem, rhs, zero);
        llvm::Value* finalMod = builder->CreateAdd(r, adjustment, "mod_oberon");

        lastValue = finalMod;
        break;
    }

    case Op::And:
        lastValue = builder->CreateLogicalAnd(lhs, rhs, "and");
        break;
    case Op::Or:
        lastValue = builder->CreateLogicalOr(lhs, rhs, "or");
        break;

    case Op::Eq:
        if (isInt) {
            lastValue = builder->CreateICmpEQ(lhs, rhs, "eq");
        }
        else if (isReal) {
            lastValue = builder->CreateFCmpOEQ(lhs, rhs, "feq");
        }
        else {
            lastValue = builder->CreateICmpEQ(lhs, rhs, "eq");
        }
        break;
    case Op::Neq:
        if (isInt) {
            lastValue = builder->CreateICmpNE(lhs, rhs, "neq");
        }
        else if (isReal) {
            lastValue = builder->CreateFCmpUNE(lhs, rhs, "fneq");
        }
        else {
            lastValue = builder->CreateICmpNE(lhs, rhs, "neq");
        }
        break;
    case Op::Lt:
        if (isInt) {
            lastValue = builder->CreateICmpSLT(lhs, rhs, "lt");
        }
        else if (isReal) {
            lastValue = builder->CreateFCmpOLT(lhs, rhs, "flt");
        }
        else {
            lastValue = builder->CreateICmpSLT(lhs, rhs, "lt");
        }
        break;
    case Op::Gt:
        if (isInt) {
            lastValue = builder->CreateICmpSGT(lhs, rhs, "gt");
        }
        else if (isReal) {
            lastValue = builder->CreateFCmpOGT(lhs, rhs, "fgt");
        }
        else {
            lastValue = builder->CreateICmpSGT(lhs, rhs, "gt");
        }
        break;
    case Op::Lte:
        if (isInt) {
            lastValue = builder->CreateICmpSLE(lhs, rhs, "lte");
        }
        else if (isReal) {
            lastValue = builder->CreateFCmpOLE(lhs, rhs, "flte");
        }
        else {
            lastValue = builder->CreateICmpSLE(lhs, rhs, "lte");
        }
        break;
    case Op::Gte:
        if (isInt) {
            lastValue = builder->CreateICmpSGE(lhs, rhs, "gte");
        }
        else if (isReal) {
            lastValue = builder->CreateFCmpOGE(lhs, rhs, "fgte");
        }
        else {
            lastValue = builder->CreateICmpSGE(lhs, rhs, "gte");
        }
        break;

    case Op::Is:
        lastValue = nullptr;
        break;

    default:
        lastValue = nullptr;
        break;
    }
}

void LLVMCodegenVisitor::visit(UnaryExpression& node)
{
    if (node.constantValue.has_value()) {
        lastValue = getConstantValue(node);
        return;
    }
    node.operand->accept(*this);
    llvm::Value* operand = lastValue;

    if (!operand) {
        lastValue = nullptr;
        return;
    }
    using Op = UnaryExpression::Op;

    switch (node.op) {
    case Op::Negate:
        if (operand->getType()->isIntegerTy()) {
            lastValue = builder->CreateNeg(operand, "neg");
        }
        else {
            lastValue = builder->CreateFNeg(operand, "fneg");
        }
        break;
    case Op::Plus:
        lastValue = operand;
        break;
    case Op::Not:
        lastValue = builder->CreateNot(operand, "not");
        break;
    default:
        lastValue = nullptr;
        break;
    }
}

void LLVMCodegenVisitor::visit(IdentifierExpression& node)
{
    llvm::Value* ptr = nullptr;

    auto it = locals.find(node.name);
    if (it != locals.end()) {
        ptr = it->second;
    }
    else {
        auto* gVar = module->getNamedGlobal(node.name);
        if (gVar) {
            ptr = gVar;
        }
        else {
            auto* fn = module->getFunction(node.name);
            if (fn) {
                lastValue = fn;
                return;
            }
            lastValue = nullptr;
            return;
        }
    }

    auto tyInfo = node.resolvedType;
    if (lvalue || tyInfo->kind == TypeKind::Array || tyInfo->kind == TypeKind::Struct) {
        lastValue = ptr;
    }
    else {
        llvm::Type* ty = toLLVMType(tyInfo);
        lastValue = builder->CreateLoad(ty, ptr, node.name);
    }
}

void LLVMCodegenVisitor::visit(ArrayAccessExpression& node)
{
    bool oldLvalue = lvalue;
    lvalue = false;
    node.array->accept(*this);
    llvm::Value* arr = lastValue;
    node.index->accept(*this);
    llvm::Value* index = lastValue;
    lvalue = oldLvalue;
    if (!arr || !index) {
        lastValue = nullptr;
        return;
    }

    if (index->getType()->isIntegerTy(8)) {
        index = builder->CreateSExt(index, llvm::Type::getInt64Ty(context), "idxext");
    }
    else if (!index->getType()->isIntegerTy(64)) {
        index = builder->CreateIntCast(index, llvm::Type::getInt64Ty(context), true, "idxcast");
    }

    auto arrTypeInfo = node.array->resolvedType;
    llvm::Type* elemTy = toLLVMType(arrTypeInfo->baseType);
    if (!elemTy) {
        lastValue = nullptr;
        return;
    }

    llvm::Value* elemPtr = nullptr;
    if (arrTypeInfo->isOpenArray) {
        elemPtr = builder->CreateGEP(elemTy, arr, {index}, "elem.ptr");
    }
    else {
        llvm::Type* arrTy = toLLVMType(arrTypeInfo);
        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0);
        elemPtr = builder->CreateGEP(arrTy, arr, {zero, index}, "elem.ptr");
    }

    if (!elemPtr) {
        lastValue = nullptr;
        return;
    }
    if (lvalue) {
        lastValue = elemPtr;
    }
    else {
        lastValue = builder->CreateLoad(elemTy, elemPtr, "elem");
    }
}

static int getStructFieldIndex(const std::shared_ptr<TypeInfo>& structInfo, const std::string& fieldName)
{
    if (!structInfo) return -1;
    int index = 0;
    std::vector<TypeInfo*> chain;
    auto current = structInfo;
    while (current) {
        chain.push_back(current.get());
        current = current->baseType;
    }
    std::reverse(chain.begin(), chain.end());

    for (const auto* info : chain) {
        for (const auto& field : info->fields) {
            if (field.name == fieldName) {
                return index;
            }
            ++index;
        }
    }
    return -1;
}

void LLVMCodegenVisitor::visit(MemberAccessExpression& node)
{
    std::shared_ptr<TypeInfo> structInfo;
    auto objType = node.object->resolvedType;
    bool oldLvalue = lvalue;
    if (objType->kind == TypeKind::Struct) {
        structInfo = objType;
        lvalue = true;
    }
    else if (objType->kind == TypeKind::Pointer && objType->baseType->kind == TypeKind::Struct) {
        structInfo = objType->baseType;
        lvalue = false;
    }

    node.object->accept(*this);
    llvm::Value* basePtr = lastValue;
    lvalue = oldLvalue;
    if (!basePtr) {
        lastValue = nullptr;
        return;
    }

    int fieldIndex = getStructFieldIndex(structInfo, node.memberName);
    if (fieldIndex < 0) {
        lastValue = nullptr;
        return;
    }

    auto* structTy = llvm::cast<llvm::StructType>(toLLVMType(structInfo));
    auto* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
    auto* idxVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), fieldIndex);

    llvm::Value* fieldPtr = builder->CreateGEP(structTy, basePtr, {zero, idxVal}, "field.ptr");
    if (!fieldPtr) {
        lastValue = nullptr;
        return;
    }

    if (lvalue) {
        lastValue = fieldPtr;
    }
    else {
        llvm::Type* fieldTy = toLLVMType(node.resolvedType);
        lastValue = builder->CreateLoad(fieldTy, fieldPtr, node.memberName);
    }
}

void LLVMCodegenVisitor::visit(DereferenceExpression& node)
{
    bool oldLvalue = lvalue;
    lvalue = false;
    node.ptr->accept(*this);
    llvm::Value* ptrVal = lastValue;
    lvalue = oldLvalue;
    if (!ptrVal) {
        lastValue = nullptr;
        return;
    }

    if (!lvalue) {
        llvm::Type* pointeeTy = toLLVMType(node.resolvedType);
        lastValue = builder->CreateLoad(pointeeTy, ptrVal, "deref");
    }
}
