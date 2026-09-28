#pragma once
#include "ternet.hpp"
#include <string>

namespace ternet::types {

enum class Kind { Unknown, Null, Bool, Int, Float, String, Array, Void };

struct Type {
    Kind kind = Kind::Unknown;
    std::string name() const;
};

struct CheckError : RuntimeError { using RuntimeError::RuntimeError; };

void check(const Program& program);

} // namespace ternet::types
