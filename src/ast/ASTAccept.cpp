#include "ASTVisitor.h"
#include "ASTExpressions.h"
#include "ASTStatements.h"
#include "ASTTypes.h"
#include "ASTDeclarations.h"

namespace obould
{

std::unique_ptr<Expression> BinaryExpression::clone() const {
    std::variant<std::unique_ptr<Expression>, std::unique_ptr<Type>> clonedRight;
    if (std::holds_alternative<std::unique_ptr<Expression>>(right)) {
        clonedRight = std::get<std::unique_ptr<Expression>>(right)->clone();
    } else {
        clonedRight = std::get<std::unique_ptr<Type>>(right)->clone();
    }
    auto result = std::make_unique<BinaryExpression>(left->clone(), nullptr, op);
    result->right = std::move(clonedRight);
    return result;
}

std::unique_ptr<Type> ProcedureType::clone() const {
    std::vector<std::unique_ptr<ProcedureParameter>> clonedParams;
    for (const auto& param : parameters) {
        clonedParams.push_back(param->clone());
    }
    return std::make_unique<ProcedureType>(
        std::move(clonedParams),
        returnType ? returnType->clone() : nullptr
    );
}

std::unique_ptr<Type> StructType::clone() const {
    std::vector<std::unique_ptr<VariableDeclaration>> clonedFields;
    for (const auto& field : fields) {
        clonedFields.push_back(std::make_unique<VariableDeclaration>(
            field->name,
            field->isExported,
            field->type ? field->type->clone() : nullptr
        ));
    }
    return std::make_unique<StructType>(
        baseType ? std::make_unique<IdentifierType>(baseType->moduleName, baseType->name) : nullptr,
        std::move(clonedFields)
    );
}

std::unique_ptr<Expression> ProcedureCall::clone() const {
    std::vector<std::unique_ptr<Expression>> clonedArgs;
    for (const auto& arg : args) {
        clonedArgs.push_back(arg->clone());
    }
    auto result = std::make_unique<ProcedureCall>(
        procedureName ? procedureName->clone() : nullptr,
        std::move(clonedArgs)
    );
    result->isTypeGuard = isTypeGuard;
    return result;
}


// Expressions
void IntegerLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void RealLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void BooleanLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void StringLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void Nil::accept(ASTVisitor& v) { v.visit(*this); }
void BinaryExpression::accept(ASTVisitor& v) { v.visit(*this); }
void UnaryExpression::accept(ASTVisitor& v) { v.visit(*this); }
void IdentifierExpression::accept(ASTVisitor& v) { v.visit(*this); }
void ArrayAccessExpression::accept(ASTVisitor& v) { v.visit(*this); }
void MemberAccessExpression::accept(ASTVisitor& v) { v.visit(*this); }
void DereferenceExpression::accept(ASTVisitor& v) { v.visit(*this); }

// Statements
void ProcedureCall::accept(ASTVisitor& v) { v.visit(*this); }
void AssignmentStatement::accept(ASTVisitor& v) { v.visit(*this); }
void IfStatement::accept(ASTVisitor& v) { v.visit(*this); }
void WhileStatement::accept(ASTVisitor& v) { v.visit(*this); }
void WhileBranch::accept(ASTVisitor& v) { v.visit(*this); }
void DoWhileStatement::accept(ASTVisitor& v) { v.visit(*this); }
void ForStatement::accept(ASTVisitor& v) { v.visit(*this); }
void SwitchStatement::accept(ASTVisitor& v) { v.visit(*this); }
void SwitchCase::accept(ASTVisitor& v) { v.visit(*this); }
void CaseLabel::accept(ASTVisitor& v) { v.visit(*this); }
void StatementsBlock::accept(ASTVisitor& v) { v.visit(*this); }

// Types
void IdentifierType::accept(ASTVisitor& v) { v.visit(*this); }
void ArrayType::accept(ASTVisitor& v) { v.visit(*this); }
void OpenArrayType::accept(ASTVisitor& v) { v.visit(*this); }
void StructType::accept(ASTVisitor& v) { v.visit(*this); }
void PointerType::accept(ASTVisitor& v) { v.visit(*this); }
void ProcedureType::accept(ASTVisitor& v) { v.visit(*this); }

// Declarations
void ConstantDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void TypeDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void VariableDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ProcedureDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ProcedureParameter::accept(ASTVisitor& v) { v.visit(*this); }
void ConstantDeclarations::accept(ASTVisitor& v) { v.visit(*this); }
void TypeDeclarations::accept(ASTVisitor& v) { v.visit(*this); }
void VariableDeclarations::accept(ASTVisitor& v) { v.visit(*this); }
void DeclarationsBlock::accept(ASTVisitor& v) { v.visit(*this); }
void Import::accept(ASTVisitor& v) { v.visit(*this); }
void Module::accept(ASTVisitor& v) { v.visit(*this); }
}
