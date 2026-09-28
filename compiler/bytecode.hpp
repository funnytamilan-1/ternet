#pragma once
#include "ternet.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace ternet::bytecode {

enum class Op : std::uint8_t {
    Halt, Const, Load, Store, Pop, Dup,
    Add, Sub, Mul, Div, Mod, Neg, Not,
    Eq, Ne, Lt, Le, Gt, Ge, And, Or,
    MakeArray, MakeObject, Index, GetMember, SetMember,
    Jump, JumpIfFalse, JumpIfTrue,
    Print, WebWrite, Call, Return
};

struct Instruction {
    Op op;
    std::int32_t operand = 0;
};

struct FunctionInfo {
    std::string name;
    std::vector<std::string> params;
    std::int32_t entry = 0;
};

struct Chunk {
    std::vector<Instruction> code;
    std::vector<Value> constants;
    std::vector<std::string> names;
    std::vector<FunctionInfo> functions;
};

struct CompileError : RuntimeError {
    using RuntimeError::RuntimeError;
};

Chunk compile(const Program& program);
void verify(const Chunk& chunk);
void write(const Chunk& chunk, const std::string& path);
Chunk read(const std::string& path);
Value execute(const Chunk& chunk);

} // namespace ternet::bytecode
