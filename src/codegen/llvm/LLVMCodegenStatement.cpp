#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


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
    auto* funcType = createFunctionType(procTypeInfo);

    const auto& params = procTypeInfo->parameters;
    std::vector<llvm::Value*> args;
    args.reserve(node.args.size());

    for (size_t i = 0; i < node.args.size(); ++i) {
        auto& argExpr = node.args[i];
        lvalue = params[i].isReference;
        argExpr->accept(*this);
        if (isIntegerType(argExpr->resolvedType->kind) && isIntegerType(params[i].type->kind)) {
            lastValue = builder->CreateZExtOrTrunc(lastValue, toLLVMType(params[i].type));
        }
        args.push_back(lastValue);
    }
    lvalue = oldLvalue;

    auto* call = builder->CreateCall(funcType, callee, args, "call");
    lastValue = funcType->getReturnType()->isVoidTy() ? nullptr : call;
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
    // TODO: присваивание строки массиву и символа char'у надо обработать
    lvalue = false;
    node.value->accept(*this);
    if (isIntegerType(node.target->resolvedType->kind) && isIntegerType(node.value->resolvedType->kind)) {
        lastValue = builder->CreateZExtOrTrunc(lastValue, toLLVMType(node.target->resolvedType));
    }
    auto* rhs = lastValue;
    lvalue = true;
    node.target->accept(*this);
    auto* addr = lastValue;
    if (!rhs || !addr) {
        return;
    }
    builder->CreateStore(rhs, addr);
}

void LLVMCodegenVisitor::visit(IfStatement& node)
{
    lvalue = false;
    node.condition->accept(*this);
    llvm::Value* condVal = lastValue;
    if (!condVal) return;
    condVal = builder->CreateTruncOrBitCast(condVal, llvm::Type::getInt1Ty(context), "ifcond");

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

        branch->condition->accept(*this);

        auto* condVal = lastValue;
        if (!condVal) {
             condVal = llvm::ConstantInt::getFalse(context);
        }
        condVal = builder->CreateTruncOrBitCast(condVal, llvm::Type::getInt1Ty(context), "cond");

        auto* bodyBB = llvm::BasicBlock::Create(context, "while.body", currentFunction);

        llvm::BasicBlock* nextCondBB = nullptr;
        if (isLastBranch) {
            nextCondBB = afterBB;
        } else {
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
    node.condition->accept(*this);

    llvm::Value* condVal = lastValue;
    if (!condVal) {
        condVal = llvm::ConstantInt::getFalse(context);
    }
    condVal = builder->CreateTruncOrBitCast(condVal, llvm::Type::getInt1Ty(context), "cond");

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
    auto* counterType = startV->getType();
    llvm::Value* stepV = nullptr;
    bool isNegativeStep = false;

    if (node.step) {
        node.step->accept(*this);
        stepV = lastValue;
        if (auto* c = llvm::dyn_cast<llvm::ConstantInt>(stepV)) {
            if (c->isNegative()) isNegativeStep = true;
        }
        // TODO: есть ли вариант получше?
        stepV = builder->CreateZExtOrTrunc(stepV, counterType);
    } else {
        stepV = llvm::ConstantInt::get(counterType, 1, true);
    }
    builder->CreateStore(startV, counterPtr);

    auto* condBB = llvm::BasicBlock::Create(context, "for.cond", currentFunction);
    auto* bodyBB = llvm::BasicBlock::Create(context, "for.body", currentFunction);
    auto* stepBB = llvm::BasicBlock::Create(context, "for.step", currentFunction);
    auto* afterBB = llvm::BasicBlock::Create(context, "for.end", currentFunction);

    builder->CreateBr(condBB);
    builder->SetInsertPoint(condBB);

    auto* cur = builder->CreateLoad(counterType, counterPtr, node.counterName);
    auto pred = isNegativeStep ? llvm::CmpInst::ICMP_SGE : llvm::CmpInst::ICMP_SLE;
    auto* cond = builder->CreateICmp(pred, cur, endV, "forcond");
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

void LLVMCodegenVisitor::visit(SwitchStatement& node)
{
    lvalue = false;
    node.selector->accept(*this);
    auto* selectorVal = lastValue;
    if (!selectorVal) return;
    auto* selTy = llvm::Type::getInt64Ty(context);
    selectorVal = builder->CreateZExtOrTrunc(selectorVal, selTy, "switch.sel");

    auto* endBB = llvm::BasicBlock::Create(context, "switch.end");
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

        for (const auto& labelPtr : casePtr->labels) {
            auto startConst = labelPtr->value->constantValue;
            int64_t startVal = std::get<int64_t>(*startConst);
            int64_t endVal = startVal;

            if (labelPtr->endValue) {
                endVal = std::get<int64_t>(*labelPtr->endValue->constantValue);
            }
            if (endVal < startVal) std::swap(startVal, endVal);

            llvm::Value* labelCond = nullptr;
            if (startVal == endVal) {
                auto* val = llvm::ConstantInt::get(selTy, startVal, true);
                labelCond = builder->CreateICmpEQ(selectorVal, val);
            } else {
                auto* low = llvm::ConstantInt::get(selTy, startVal, true);
                auto* high = llvm::ConstantInt::get(selTy, endVal, true);
                auto* ge = builder->CreateICmpSGE(selectorVal, low);
                auto* le = builder->CreateICmpSLE(selectorVal, high);
                labelCond = builder->CreateAnd(ge, le);
            }

            if (caseCond) {
                caseCond = builder->CreateOr(caseCond, labelCond);
            } else {
                caseCond = labelCond;
            }
        }
        builder->CreateCondBr(caseCond, bodyBB, nextCheckBB);

        builder->SetInsertPoint(bodyBB);
        casePtr->body->accept(*this);
        builder->CreateBr(endBB);
        checkBB = nextCheckBB;
    }
    endBB->insertInto(currentFunction);
    builder->SetInsertPoint(endBB);
}

void LLVMCodegenVisitor::visit(SwitchCase& node) {}
void LLVMCodegenVisitor::visit(CaseLabel& node) {}
