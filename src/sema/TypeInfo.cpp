#include "TypeInfo.h"


static bool isBaseTypeOf(const TypeInfo* base, const TypeInfo* derived)
{
    const TypeInfo* current = derived;
    while (current) {
        if (base == current) {
            return true;
        }
        current = current->baseType.get();
    }
    return false;
}

bool TypeInfo::isAssignableFrom(const std::shared_ptr<TypeInfo>& other) const
{
    if (!other) return false;

    if (this == other.get()) {
        return true;
    }

    if (this->kind == TypeKind::Void || other->kind == TypeKind::Void) {
        return false;
    }

    if (this->kind == TypeKind::i64 && other->kind == TypeKind::Byte) {
        return true;
    }

    if (this->kind == TypeKind::Char && other->kind == TypeKind::String) {
        return other->length == 1;
    }

    if (this->kind == TypeKind::Array && other->kind == TypeKind::String) {
        if (this->baseType && this->baseType->kind == TypeKind::Char) {
            return this->length >= other->length;
        }
    }

    if (this->kind != other->kind) {
        return false;
    }

    switch (this->kind) {
    case TypeKind::Pointer:
        if (other->kind == TypeKind::Nil) return true;
        if (!this->baseType || !other->baseType) return false;
        return isBaseTypeOf(this->baseType.get(), other->baseType.get());

    case TypeKind::Struct:
        return isBaseTypeOf(this, other.get());

    case TypeKind::Array:
        return false;

    case TypeKind::Procedure:
        if (!this->returnType->isAssignableFrom(other->returnType)) return false;
        if (this->parameters.size() != other->parameters.size()) return false;

        for (size_t i = 0; i < this->parameters.size(); ++i) {
            const auto& p1 = this->parameters[i];
            const auto& p2 = other->parameters[i];

            if (p1.isReference != p2.isReference) return false;

            if (p1.isReference) {
                if (p1.type.get() != p2.type.get()) return false;
            }
            else {
                if (!p1.type->isAssignableFrom(p2.type)) return false;
            }
        }
    default:
        return true;
    }
}
