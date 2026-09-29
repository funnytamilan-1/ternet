#pragma once
#include "ternet.hpp"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ternet::types {

enum class Kind {
    Unknown,
    Null,
    Bool,
    Int,
    Float,
    Char,
    Byte,
    String,
    Array,
    List,
    Map,
    Tuple,
    Function,
    Void,
    Never,
    Option,
    Result,
    User,
    Class,
    Trait,
    GenericParam
};

struct Type {
    Kind kind = Kind::Unknown;
    std::string custom;
    std::vector<Type> args;

    std::string name() const;

    static Type unknown() { return {Kind::Unknown, {}, {}}; }
    static Type simple(Kind k) { return {k, {}, {}}; }
    static Type int_type() { return {Kind::Int, {}, {}}; }
    static Type float_type() { return {Kind::Float, {}, {}}; }
    static Type bool_type() { return {Kind::Bool, {}, {}}; }
    static Type string_type() { return {Kind::String, {}, {}}; }
    static Type void_type() { return {Kind::Void, {}, {}}; }
    static Type null_type() { return {Kind::Null, {}, {}}; }
    static Type never_type() { return {Kind::Never, {}, {}}; }

    static Type user(std::string n) { return {Kind::User, std::move(n), {}}; }
    static Type class_type(std::string n) { return {Kind::Class, std::move(n), {}}; }
    static Type trait_type(std::string n) { return {Kind::Trait, std::move(n), {}}; }
    static Type generic_param(std::string n) { return {Kind::GenericParam, std::move(n), {}}; }

    static Type list(Type element) { return {Kind::List, {}, {std::move(element)}}; }
    static Type array(Type element) { return {Kind::Array, {}, {std::move(element)}}; }
    static Type map(Type key, Type value) { return {Kind::Map, {}, {std::move(key), std::move(value)}}; }
    static Type tuple(std::vector<Type> elements) { return {Kind::Tuple, {}, std::move(elements)}; }
    static Type option(Type value) { return {Kind::Option, {}, {std::move(value)}}; }
    static Type result(Type ok, Type error) { return {Kind::Result, {}, {std::move(ok), std::move(error)}}; }
    static Type func(std::vector<Type> params, Type ret) {
        std::vector<Type> all = std::move(params);
        all.push_back(std::move(ret));
        return {Kind::Function, {}, std::move(all)};
    }

    bool is_unknown() const { return kind == Kind::Unknown; }
    bool is_numeric() const { return kind == Kind::Int || kind == Kind::Float; }
    bool is_integer() const { return kind == Kind::Int; }
};

struct CheckError : RuntimeError {
    using RuntimeError::RuntimeError;
};

void check(const Program& program);

} // namespace ternet::types
