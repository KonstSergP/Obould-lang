#pragma once
#include <memory>

class SemanticAnalyzer;
class Expression;
class Type;


enum class TypeKind
{
    i64, Byte, f64,  Bool, Char, String, Void,
    Array, Struct, Pointer, Procedure
};


struct TypeInfo
{
    TypeKind kind;

    // String + Array
    int length;

    // Pointer + Array + Struct
    TypeInfo* baseType = nullptr;

    // Array
    bool isOpenArray = false;


    bool isAssignableFrom(const std::shared_ptr<TypeInfo>& other) const;
};
