#include "SemanticAnalyzer.h"
#include "TypeInfo.h"


bool SemanticAnalyzer::analyze(Module& module)
{
    errors.clear();
    module.accept(*this);
    return errors.empty();
}

const std::vector<std::string>& SemanticAnalyzer::getErrors() const
{
    return errors;
}

void SemanticAnalyzer::addError(const std::string& error)
{
    errors.push_back(error);
}

std::shared_ptr<TypeInfo> SemanticAnalyzer::getPolymorphicBase(Expression* expr)
{
    auto type = expr->resolvedType;
    if (!type) return nullptr;

    if (type->kind == TypeKind::Pointer && type->baseType->kind == TypeKind::Struct) {
        return type->baseType;
    }

    if (type->kind == TypeKind::Struct) {
        if (auto* idExpr = dynamic_cast<IdentifierExpression*>(expr)) {
            auto* sym = symbolTable.lookupSymbol(idExpr->name);
            if (sym && sym->isReference) {
                return type;
            }
        }
    }

    return nullptr;
}
