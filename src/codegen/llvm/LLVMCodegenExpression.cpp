#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"
#include <llvm/IR/Module.h>


namespace obould
{
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
    lengths[node.resolvedType.get()] = builder->getInt64(node.value.length() + 1);
    lastValue = getConstantValue(node);
}

void LLVMCodegenVisitor::visit(Nil& node)
{
    lastValue = llvm::ConstantPointerNull::get(builder->getPtrTy());
}

void LLVMCodegenVisitor::visit(BinaryExpression& node)
{
    using Op = BinaryExpression::Op;
    if (node.constantValue.has_value()) {
        lastValue = getConstantValue(node);
        return;
    }
    node.left->accept(*this);
    auto* lhs = lastValue;

    if (node.op == Op::Is) {
        auto lType = node.left->resolvedType;
        if (lType->kind == TypeKind::Pointer) lType = lType->baseType;
        auto* typeTagPtrAddr = builder->CreateStructGEP(toLLVMType(lType), lhs, 0);
        auto* lDesc = builder->CreateLoad(builder->getPtrTy(), typeTagPtrAddr, "obj.tag");

        auto& rType = std::get<std::unique_ptr<Type>>(node.right)->resolvedType;
        auto* rDesc = descriptors[rType.get()];

        makeStructCastCheck(lDesc, rDesc, rType->depth);
        return;
    }

    auto& right = std::get<std::unique_ptr<Expression>>(node.right);

    if (node.op == Op::And) {
        auto* func = builder->GetInsertBlock()->getParent();
        auto* originalBB = builder->GetInsertBlock();
        auto* evalRightBB = llvm::BasicBlock::Create(context, "and.rhs", func);
        auto* mergeBB = llvm::BasicBlock::Create(context, "and.merge", func);
        builder->CreateCondBr(lhs, evalRightBB, mergeBB);

        builder->SetInsertPoint(evalRightBB);
        right->accept(*this);
        auto* rightVal = lastValue;
        auto* rightEndBB = builder->GetInsertBlock();
        builder->CreateBr(mergeBB);
        builder->SetInsertPoint(mergeBB);

        auto* phi = builder->CreatePHI(builder->getInt1Ty(), 2, "and.res");
        phi->addIncoming(rightVal, rightEndBB);
        phi->addIncoming(builder->getFalse(), originalBB);
        lastValue = phi;
        return;
    }
    if (node.op == Op::Or) {
        auto* func = builder->GetInsertBlock()->getParent();
        auto* originalBB = builder->GetInsertBlock();
        auto* evalRightBB = llvm::BasicBlock::Create(context, "or.rhs", func);
        auto* mergeBB = llvm::BasicBlock::Create(context, "or.merge", func);
        builder->CreateCondBr(lhs, mergeBB, evalRightBB);

        builder->SetInsertPoint(evalRightBB);
        right->accept(*this);
        auto* rightVal = lastValue;
        auto* rightEndBB = builder->GetInsertBlock();
        builder->CreateBr(mergeBB);
        builder->SetInsertPoint(mergeBB);

        auto* phi = builder->CreatePHI(builder->getInt1Ty(), 2, "or.res");
        phi->addIncoming(builder->getTrue(), originalBB);
        phi->addIncoming(rightVal, rightEndBB);
        lastValue = phi;
        return;
    }

    right->accept(*this);
    auto* rhs = lastValue;
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

    if (isIntegerType(resultType->kind)) {
        if (node.left->resolvedType->kind == TypeKind::i64 && right->resolvedType->kind == TypeKind::Byte) {
            rhs = builder->CreateZExt(rhs, builder->getInt64Ty());
        }
        else if (node.left->resolvedType->kind == TypeKind::Byte && right->resolvedType->kind == TypeKind::i64) {
            lhs = builder->CreateZExt(lhs, builder->getInt64Ty());
        }
    }
    else if (node.left->resolvedType->kind == TypeKind::String) { // TODO: для строк ни логики, ни toLLMVType
        lhs = builder->CreateLoad(toLLVMType(right->resolvedType), lhs);
    }
    else if (right->resolvedType->kind == TypeKind::String) {
        rhs = builder->CreateLoad(toLLVMType(node.left->resolvedType), rhs);
    }

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
    if (node.constantValue.has_value()) {
        lastValue = getConstantValue(node);
        return;
    }
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
        index = builder->CreateSExt(index, builder->getInt64Ty(), "idxext");
    }
    else if (!index->getType()->isIntegerTy(64)) {
        index = builder->CreateIntCast(index, builder->getInt64Ty(), true, "idxcast");
    }

    auto arrTypeInfo = node.array->resolvedType;
    auto* elemTy = toLLVMType(arrTypeInfo->baseType);
    if (!elemTy) {
        lastValue = nullptr;
        return;
    }

    llvm::Value* elemPtr = nullptr;
    if (arrTypeInfo->isOpenArray) {
        elemPtr = builder->CreateGEP(elemTy, arr, {index}, "elem.ptr");
    }
    else {
        auto* arrTy = toLLVMType(arrTypeInfo);
        auto* zero = builder->getInt64(0);
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
    int index = 1; // first field is pointer to struct descriptor
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
    auto* basePtr = lastValue;
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

    llvm::Value* fieldPtr = builder->CreateStructGEP(toLLVMType(structInfo), basePtr, fieldIndex, "field.ptr");
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
    auto* ptrVal = lastValue;
    lvalue = oldLvalue;
    if (!ptrVal) {
        lastValue = nullptr;
        return;
    }

    if (!lvalue) {
        auto* pointeeTy = toLLVMType(node.resolvedType);
        lastValue = builder->CreateLoad(pointeeTy, ptrVal, "deref");
    }
    else {
        lastValue = ptrVal;
    }
}

void LLVMCodegenVisitor::visit(QualifiedNameNode& node)
{
    if (auto id = std::get_if<std::unique_ptr<IdentifierExpression>>(&node.realisation)) {
        (*id)->accept(*this);
    }
    else if (auto member = std::get_if<std::unique_ptr<MemberAccessExpression>>(&node.realisation)) {
        (*member)->accept(*this);
    }
    else {
        lastValue = nullptr;
    }
}
}
