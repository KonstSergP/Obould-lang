#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


namespace obould
{
void LLVMCodegenVisitor::visit(ProcedureCall& node)
{
    bool oldLvalue = lvalue;
    lvalue = false;
    node.procedureName->accept(*this);
    auto* callee = lastValue;
    if (!callee) {
        lastValue = nullptr;
        return;
    }
    auto procTypeInfo = node.procedureName->resolvedType;

    if (node.isTypeGuard) {
        auto lType = node.procedureName->resolvedType;
        if (lType->kind == TypeKind::Pointer) lType = lType->baseType;
        auto* typeTagPtrAddr = builder->CreateStructGEP(toLLVMType(lType), callee, 0);
        auto* lDesc = builder->CreateLoad(builder->getPtrTy(), typeTagPtrAddr, "obj.tag");

        auto& rType = node.args[0]->resolvedType;
        auto* rDesc = descriptors[rType.get()];

        makeStructCastCheck(lDesc, rDesc, rType->depth);

        auto* failBB = llvm::BasicBlock::Create(context, "guard.fail", currentFunction);
        auto* succBB = llvm::BasicBlock::Create(context, "guard.succ", currentFunction);
        builder->CreateCondBr(lastValue, succBB, failBB);

        builder->SetInsertPoint(failBB);
        auto abortFunc = module->getOrInsertFunction("abort", llvm::FunctionType::get(builder->getVoidTy(), false));
        builder->CreateCall(abortFunc);
        builder->CreateUnreachable();

        builder->SetInsertPoint(succBB);
        lastValue = callee;
    }
    else {
        auto* funcType = createFunctionType(procTypeInfo);

        const auto& params = procTypeInfo->parameters;
        std::vector<llvm::Value*> args, hiddenParams;

        for (size_t i = 0; i < node.args.size(); ++i) {
            auto& argExpr = node.args[i];
            lvalue = params[i].isReference;
            argExpr->accept(*this);

            if (!params[i].isReference
                && isIntegerType(argExpr->resolvedType->kind)
                && isIntegerType(params[i].type->kind)) {
                lastValue = builder->CreateZExtOrTrunc(lastValue, toLLVMType(params[i].type));
            }
            args.push_back(lastValue);

            auto factType = argExpr->resolvedType;
            auto formalType = params[i].type;
            while (formalType->isOpenArray) {
                auto* lenVal = lengths[factType.get()];
                if (lenVal->getType()->isPointerTy()) {
                    lenVal = builder->CreateLoad(builder->getInt64Ty(), lenVal);
                }
                hiddenParams.push_back(lenVal);

                formalType = formalType->baseType;
                factType = factType->baseType;
            }
        }
        args.insert(args.end(), hiddenParams.begin(), hiddenParams.end());

        auto* call = builder->CreateCall(funcType, callee, args, "call");
        lastValue = funcType->getReturnType()->isVoidTy() ? nullptr : call;
    }
    lvalue = oldLvalue;
}

void LLVMCodegenVisitor::visit(StatementsBlock& node)
{
    for (const auto& stmt : node.statements) {
        lastValue = nullptr;
        stmt->accept(*this);
    }
}

void LLVMCodegenVisitor::visit(AssignmentStatement& node)
{
    lvalue = false;
    node.value->accept(*this);
    auto* rhs = lastValue;
    lvalue = true;
    node.target->accept(*this);
    auto* lhs = lastValue;
    if (!rhs || !lhs) {
        return;
    }

    if (isIntegerType(node.target->resolvedType->kind) && isIntegerType(node.value->resolvedType->kind)) {
        bool srcSigned = node.value->resolvedType->kind == TypeKind::i64;
        rhs = builder->CreateIntCast(rhs, toLLVMType(node.target->resolvedType), srcSigned);
    }
    else if (node.value->resolvedType->kind == TypeKind::String) {
        if (node.target->resolvedType->kind == TypeKind::Char) {
            rhs = builder->CreateLoad(toLLVMType(node.target->resolvedType), rhs);
        }
        else { // array of chars
            lhs = builder->CreateBitCast(lhs, builder->getPtrTy());
            llvm::Value* sizeVal = llvm::ConstantInt::get(builder->getInt64Ty(),
                                                          node.value->resolvedType->length + 1); // + нулевой символ
            builder->CreateMemCpy(lhs, llvm::MaybeAlign(1), rhs, llvm::MaybeAlign(1), sizeVal);
            return;
        }
    }
    else if (node.target->resolvedType->kind == TypeKind::Array) {
        auto* objType = toLLVMType(node.target->resolvedType);
        auto sizeBytes = module->getDataLayout().getTypeAllocSize(objType);
        auto* sizeVal = llvm::ConstantInt::get(builder->getInt64Ty(), sizeBytes);

        builder->CreateMemCpy(lhs, llvm::MaybeAlign(1), rhs, llvm::MaybeAlign(1), sizeVal);
        return;
    }
    else if (node.target->resolvedType->kind == TypeKind::Struct) {
        auto* structTy = llvm::cast<llvm::StructType>(toLLVMType(node.target->resolvedType));
        auto* layout = module->getDataLayout().getStructLayout(structTy);
        uint64_t totalSize = module->getDataLayout().getTypeAllocSize(structTy);

        if (structTy->getNumElements() > 1) {
            uint64_t payloadOffset = layout->getElementOffset(1);
            uint64_t payloadSize = totalSize - payloadOffset;

            auto* dstPayload = builder->CreateConstGEP1_64(builder->getInt8Ty(), lhs, payloadOffset);
            auto* srcPayload = builder->CreateConstGEP1_64(builder->getInt8Ty(), rhs, payloadOffset);

            builder->CreateMemCpy(dstPayload, layout->getAlignment(), srcPayload, layout->getAlignment(),
                                  llvm::ConstantInt::get(builder->getInt64Ty(), payloadSize));
        }
        return;
    }
    builder->CreateStore(rhs, lhs);
}

void LLVMCodegenVisitor::visit(IfStatement& node)
{
    lvalue = false;
    node.condition->accept(*this);
    llvm::Value* condVal = lastValue;
    if (!condVal) return;
    condVal = builder->CreateTruncOrBitCast(condVal, builder->getInt1Ty(), "ifcond");

    auto* thenBB = llvm::BasicBlock::Create(context, "then", currentFunction);
    auto* elseBB = llvm::BasicBlock::Create(context, "else");
    auto* mergeBB = llvm::BasicBlock::Create(context, "merge");

    builder->CreateCondBr(condVal, thenBB, node.elseBranch ? elseBB : mergeBB);

    builder->SetInsertPoint(thenBB);
    node.thenBranch->accept(*this);
    builder->CreateBr(mergeBB);

    if (node.elseBranch) {
        elseBB->insertInto(currentFunction);
        builder->SetInsertPoint(elseBB);
        node.elseBranch->accept(*this);
        builder->CreateBr(mergeBB);
    }
    mergeBB->insertInto(currentFunction);
    builder->SetInsertPoint(mergeBB);
}

void LLVMCodegenVisitor::visit(WhileStatement& node)
{
    auto* afterBB = llvm::BasicBlock::Create(context, "while.end");
    auto* headBB = llvm::BasicBlock::Create(context, "while.cond", currentFunction);
    auto* currentCondBB = headBB;
    builder->CreateBr(headBB);

    for (size_t i = 0; i < node.branches.size(); ++i) {
        bool isLastBranch = (i == node.branches.size() - 1);
        auto& branch = node.branches[i];

        currentCondBB->insertInto(currentFunction);
        builder->SetInsertPoint(currentCondBB);

        lvalue = false;
        branch->condition->accept(*this);

        auto* condVal = lastValue;
        if (!condVal) {
            condVal = llvm::ConstantInt::getFalse(context);
        }
        condVal = builder->CreateTruncOrBitCast(condVal, builder->getInt1Ty(), "cond");

        auto* bodyBB = llvm::BasicBlock::Create(context, "while.body", currentFunction);

        llvm::BasicBlock* nextCondBB = nullptr;
        if (isLastBranch) {
            nextCondBB = afterBB;
        }
        else {
            nextCondBB = llvm::BasicBlock::Create(context, "while.cond");
        }
        builder->CreateCondBr(condVal, bodyBB, nextCondBB);

        builder->SetInsertPoint(bodyBB);
        branch->body->accept(*this);
        builder->CreateBr(headBB);

        currentCondBB = nextCondBB;
    }
    afterBB->insertInto(currentFunction);
    builder->SetInsertPoint(afterBB);
}

void LLVMCodegenVisitor::visit(WhileBranch& node) {}

void LLVMCodegenVisitor::visit(DoWhileStatement& node)
{
    auto* bodyBB = llvm::BasicBlock::Create(context, "dowhile.body", currentFunction);
    auto* condBB = llvm::BasicBlock::Create(context, "dowhile.cond");
    auto* afterBB = llvm::BasicBlock::Create(context, "dowhile.end");

    builder->CreateBr(bodyBB);
    builder->SetInsertPoint(bodyBB);
    node.body->accept(*this);

    builder->CreateBr(condBB);
    condBB->insertInto(currentFunction);
    builder->SetInsertPoint(condBB);
    lvalue = false;
    node.condition->accept(*this);

    llvm::Value* condVal = lastValue;
    if (!condVal) {
        condVal = llvm::ConstantInt::getFalse(context);
    }
    condVal = builder->CreateTruncOrBitCast(condVal, builder->getInt1Ty(), "cond");

    builder->CreateCondBr(condVal, bodyBB, afterBB);
    afterBB->insertInto(currentFunction);
    builder->SetInsertPoint(afterBB);
}

void LLVMCodegenVisitor::visit(ForStatement& node)
{
    lvalue = false;
    node.rangeStart->accept(*this);
    auto* startV = lastValue;
    node.rangeEnd->accept(*this);
    auto* endV = lastValue;
    if (!startV || !endV) return;

    auto* counterPtr = locals[node.counterName];
    auto* counterType = builder->getInt64Ty();
    llvm::Value* stepV = nullptr;
    bool isNegativeStep = false;

    if (node.step) {
        node.step->accept(*this);
        auto* c = llvm::dyn_cast<llvm::ConstantInt>(lastValue);
        if (c->isNegative()) isNegativeStep = true;
        stepV = builder->CreateSExtOrTrunc(stepV, counterType);
    }
    else {
        stepV = llvm::ConstantInt::get(counterType, 1, true);
    }
    startV = builder->CreateSExtOrTrunc(startV, counterType);
    endV = builder->CreateSExtOrTrunc(endV, counterType);
    builder->CreateStore(startV, counterPtr);

    auto* condBB = llvm::BasicBlock::Create(context, "for.cond", currentFunction);
    auto* bodyBB = llvm::BasicBlock::Create(context, "for.body", currentFunction);
    auto* stepBB = llvm::BasicBlock::Create(context, "for.step", currentFunction);
    auto* afterBB = llvm::BasicBlock::Create(context, "for.end", currentFunction);

    builder->CreateBr(condBB);
    builder->SetInsertPoint(condBB);

    auto* cur = builder->CreateLoad(counterType, counterPtr, node.counterName);
    auto pred = isNegativeStep ? llvm::CmpInst::ICMP_SGE : llvm::CmpInst::ICMP_SLE;
    auto* cond = builder->CreateICmp(pred, cur, endV);
    builder->CreateCondBr(cond, bodyBB, afterBB);

    bodyBB->insertInto(currentFunction);
    builder->SetInsertPoint(bodyBB);
    node.body->accept(*this);

    builder->CreateBr(stepBB);
    stepBB->insertInto(currentFunction);
    builder->SetInsertPoint(stepBB);

    llvm::Value* next = builder->CreateAdd(cur, stepV, "nextVal");
    builder->CreateStore(next, counterPtr);
    builder->CreateBr(condBB);

    afterBB->insertInto(currentFunction);
    builder->SetInsertPoint(afterBB);
}

static int64_t getLabelValue(const ConstValue& val)
{
    if (std::holds_alternative<int64_t>(val)) {
        return std::get<int64_t>(val);
    }
    if (std::holds_alternative<std::string>(val)) {
        const auto& s = std::get<std::string>(val);
        if (!s.empty()) return static_cast<uint8_t>(s[0]);
    }
    return 0;
}

void LLVMCodegenVisitor::visit(SwitchStatement& node)
{
    lvalue = false;
    node.selector->accept(*this);
    auto* selector = lastValue;
    if (!selector) return;

    llvm::Type* selectorType;
    if (selector->getType()->isPointerTy()) {
        selectorType = builder->getPtrTy();
    }
    else {
        selectorType = builder->getInt64Ty();
        auto origType = node.selector->resolvedType->kind;
        bool isSignedCast = origType != TypeKind::Byte && origType != TypeKind::Char;
        selector = builder->CreateIntCast(selector, selectorType, isSignedCast, "switch.sel");
    }

    auto* endBB = llvm::BasicBlock::Create(context, "switch.end", currentFunction);
    auto* checkBB = llvm::BasicBlock::Create(context, "switch.check", currentFunction);
    builder->CreateBr(checkBB);

    for (size_t i = 0; i < node.cases.size(); ++i) {
        auto& casePtr = node.cases[i];
        builder->SetInsertPoint(checkBB);

        auto* bodyBB = llvm::BasicBlock::Create(context, "switch.body", currentFunction);
        auto* nextCheckBB = (i == node.cases.size() - 1)
                                ? endBB
                                : llvm::BasicBlock::Create(context, "switch.check", currentFunction);
        llvm::Value* caseCond = nullptr;

        for (const auto& lbl : casePtr->labels) {
            if (auto type = node.selector->resolvedType->kind; type == TypeKind::Struct || type == TypeKind::Pointer) {
                auto lType = node.selector->resolvedType;
                if (lType->kind == TypeKind::Pointer) lType = lType->baseType;
                auto* typeTagPtrAddr = builder->CreateStructGEP(toLLVMType(lType), selector, 0);
                auto* lDesc = builder->CreateLoad(builder->getPtrTy(), typeTagPtrAddr, "obj.tag");

                auto& rType = lbl->value->resolvedType;
                auto* rDesc = descriptors[rType.get()];

                makeStructCastCheck(lDesc, rDesc, rType->depth);
                caseCond = lastValue;
            }
            else {
                int64_t startVal = getLabelValue(*lbl->value->constantValue);
                int64_t endVal = startVal;
                if (lbl->endValue) {
                    endVal = getLabelValue(*lbl->endValue->constantValue);
                }
                if (endVal < startVal) std::swap(startVal, endVal);
                llvm::Value* labelCond = nullptr;
                if (startVal == endVal) {
                    auto* val = llvm::ConstantInt::get(selectorType, startVal, true);
                    labelCond = builder->CreateICmpEQ(selector, val);
                }
                else {
                    auto* low = llvm::ConstantInt::get(selectorType, startVal, true);
                    auto* high = llvm::ConstantInt::get(selectorType, endVal, true);
                    auto* ge = builder->CreateICmpSGE(selector, low);
                    auto* le = builder->CreateICmpSLE(selector, high);
                    labelCond = builder->CreateAnd(ge, le);
                }

                if (caseCond) {
                    caseCond = builder->CreateOr(caseCond, labelCond);
                }
                else {
                    caseCond = labelCond;
                }
            }
        }
        builder->CreateCondBr(caseCond, bodyBB, nextCheckBB);

        builder->SetInsertPoint(bodyBB);
        casePtr->body->accept(*this);
        builder->CreateBr(endBB);
        checkBB = nextCheckBB;
    }
    builder->SetInsertPoint(endBB);
}

void LLVMCodegenVisitor::visit(SwitchCase& node) {}
void LLVMCodegenVisitor::visit(CaseLabel& node) {}
}
