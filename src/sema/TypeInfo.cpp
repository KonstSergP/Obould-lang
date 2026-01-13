#include "TypeInfo.h"


bool TypeInfo::isBaseTypeOf(const std::shared_ptr<TypeInfo>& other) const
{
    if (!other) return false;
    auto current = other.get();
    while (current) {
        if (this == current) {
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

    if (other->kind == TypeKind::Nil) {
        return (this->kind == TypeKind::Pointer || this->kind == TypeKind::Procedure);
    }

    if (this->kind == TypeKind::i64 && other->kind == TypeKind::Byte) {
        return true;
    }

    if (other->kind == TypeKind::String) {
        if (this->kind == TypeKind::Char) {
            return other->length == 1;
        }
        if (this->kind == TypeKind::Array && this->baseType->kind == TypeKind::Char) {
            return this->length > other->length;
        }
    }

    if (this->kind != other->kind) {
        return false;
    }

    switch (this->kind) {
    case TypeKind::Pointer:
        if (this->baseType && other->baseType) {
            return this->baseType->isBaseTypeOf(other->baseType);
        }
        return false;

    case TypeKind::Struct:
        return this->isBaseTypeOf(other);

    case TypeKind::Array:
        if (this->baseType && other->baseType) {
            if (this->baseType.get() != other->baseType.get()) {
                return false;
            }
            if (other->isOpenArray) {
                return true;
            }
            return this->length == other->length;
        }
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
