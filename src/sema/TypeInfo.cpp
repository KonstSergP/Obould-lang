#include "TypeInfo.h"


namespace obould
{
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

    if (kind == TypeKind::Void || other->kind == TypeKind::Void) {
        return false;
    }

    if (other->kind == TypeKind::Nil) {
        return kind == TypeKind::Pointer || kind == TypeKind::Procedure;
    }

    if (isIntegerType(kind) && isIntegerType(other->kind)) {
        return true;
    }

    if (other->kind == TypeKind::String) {
        if (kind == TypeKind::Char) {
            return other->length == 1;
        }
        if (kind == TypeKind::Array && baseType->kind == TypeKind::Char) {
            return length > other->length;
        }
        return false;
    }

    if (kind != other->kind) {
        return false;
    }

    switch (kind) {
    case TypeKind::Pointer:
        if (baseType && other->baseType) {
            return baseType->isBaseTypeOf(other->baseType);
        }
        return false;

    case TypeKind::Struct:
        return isBaseTypeOf(other);

    case TypeKind::Array:
        if (isOpenArray) {
            if (other->kind == TypeKind::Array) {
                return baseType.get() == other->baseType.get();
            }
        }
        return false;

    case TypeKind::Procedure:
        if (!returnType->isAssignableFrom(other->returnType)) return false;
        if (parameters.size() != other->parameters.size()) return false;

        for (size_t i = 0; i < parameters.size(); ++i) {
            const auto& p1 = parameters[i];
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

bool TypeInfo::isArrayConvertibleFrom(const std::shared_ptr<TypeInfo>& other) const
{
    if (!other) return false;
    if (kind != TypeKind::Array || other->kind != TypeKind::Array) return false;
    if (!isOpenArray) return false;

    if (baseType->isOpenArray) {
        return baseType->isArrayConvertibleFrom(other->baseType);
    }
    return baseType == other->baseType;
}
}
