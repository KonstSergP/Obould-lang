#pragma once
#include <memory>
#include <string>
#include <vector>


namespace obould
{
enum class TypeKind
{
    i64,
    Byte,
    f64,
    Bool,
    Char,
    String,
    Void,
    Nil,
    Array,
    Struct,
    Pointer,
    Procedure,
    Incomplete
};

inline bool isIntegerType(TypeKind kind)
{
    return kind == TypeKind::i64 || kind == TypeKind::Byte;
}

inline bool isRealType(TypeKind kind)
{
    return kind == TypeKind::f64;
}

inline bool isNumericType(TypeKind kind)
{
    return isIntegerType(kind) || isRealType(kind);
}

inline bool isValidVariableType(TypeKind kind)
{
    switch (kind) {
    case TypeKind::i64:
    case TypeKind::Byte:
    case TypeKind::f64:
    case TypeKind::Bool:
    case TypeKind::Char:
    case TypeKind::Array:
    case TypeKind::Struct:
    case TypeKind::Pointer:
    case TypeKind::Procedure:
        return true;
    default:
        return false;
    }
}


struct TypeInfo;

struct FieldInfo
{
    std::string name;
    std::shared_ptr<TypeInfo> type;
    bool isExported;
};

struct ParamInfo
{
    std::string name;
    std::shared_ptr<TypeInfo> type;
    bool isReference;
};

enum class BuiltinKind {
    None,
    NEW,
    LEN,
    ASSERT
};

struct TypeInfo
{
    TypeInfo() : kind(TypeKind::Void) {}

    explicit TypeInfo(const TypeKind k) : kind(k) {}

    [[nodiscard]] bool isAssignableFrom(const std::shared_ptr<TypeInfo>& other) const;
    [[nodiscard]] bool isBaseTypeOf(const std::shared_ptr<TypeInfo>& other) const;
    [[nodiscard]] bool isArrayConvertibleFrom(const std::shared_ptr<TypeInfo>& other) const;


    TypeKind kind;
    std::string name;

    // Array
    bool isOpenArray = false;

    // String + Array
    int64_t length = 0;

    // Pointer + Array + Struct
    std::shared_ptr<TypeInfo> baseType;

    // Struct
    std::string ownerModule;

    // Struct
    int64_t depth = -1;
    std::vector<FieldInfo> fields;

    // Procedure
    std::shared_ptr<TypeInfo> returnType;
    std::vector<ParamInfo> parameters;
    BuiltinKind builtin = BuiltinKind::None;
};
}
