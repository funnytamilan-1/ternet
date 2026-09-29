#pragma once
#include "ternet.hpp"
#include <string>
#include <utility>
#include <vector>

namespace ternet::types {

enum class Kind {
    Unknown, Null, Bool, Int, Float, Char, Byte, String,
    Array, Map, Tuple, Function, Void, Option, Result, User, Generic
};

struct Type {
    Kind kind = Kind::Unknown;
    std::string custom;
    std::vector<Type> args;

    std::string name() const;

    static Type unknown() { return {Kind::Unknown, {}, {}}; }
    static Type simple(Kind k) { return {k, {}, {}}; }
    static Type user(std::string n) { return {Kind::User, std::move(n), {}}; }
    static Type generic(std::string n, std::vector<Type> parameters) {
        return {Kind::Generic, std::move(n), std::move(parameters)};
    }
    static Type array(Type element) { return {Kind::Array, {}, {std::move(element)}}; }
    static Type list(Type element) { return array(std::move(element)); }
    static Type map(Type key, Type value) {
        return {Kind::Map, {}, {std::move(key), std::move(value)}};
    }
    static Type tuple(std::vector<Type> elements) {
        return {Kind::Tuple, {}, std::move(elements)};
    }
    static Type option(Type value) { return {Kind::Option, {}, {std::move(value)}}; }
    static Type result(Type ok, Type error) {
        return {Kind::Result, {}, {std::move(ok), std::move(error)}};
    }

    bool is_unknown() const { return kind == Kind::Unknown; }
    bool is_numeric() const { return kind == Kind::Int || kind == Kind::Float; }
};

struct CheckError : RuntimeError { using RuntimeError::RuntimeError; };

void check(const Program& program);

} // namespace ternet::types
