#pragma once
#include <memory>
#include <string>
#include <vector>

class SemanticAnalyzer;
class Expression;
class Type;


enum class TypeKind
{
    i64, Byte, f64, Bool, Char, String, Void,
    Array, Struct, Pointer, Procedure
};


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


struct TypeInfo
{
    TypeKind kind;

    // Array
    bool isOpenArray;

    // String + Array
    int length;

    // Pointer + Array + Struct
    std::shared_ptr<TypeInfo> baseType;

    // Struct
    std::string name;
    std::vector<FieldInfo> fields;

    // Procedure
    std::shared_ptr<TypeInfo> returnType;
    std::vector<ParamInfo> parameters;

    bool isAssignableFrom(const std::shared_ptr<TypeInfo>& other) const;
};
