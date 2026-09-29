#pragma once
#include "ternet.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace ternet::bytecode {

enum class Op : std::uint8_t {
    Halt, Const, Load, Store, Pop, Dup,
    Add, Sub, Mul, Div, Mod, Neg, Not, BitAnd, BitOr, BitXor, ShiftLeft, ShiftRight,
    Eq, Ne, Lt, Le, Gt, Ge, And, Or,
    MakeArray, MakeTuple, MakeObject, Index, SetIndex, GetMember, SetMember,
    Jump, JumpIfFalse, JumpIfTrue,
    Call, CallMethod, Return,
    Print, WebWrite,
    PushCatch, PopCatch, Throw
};

struct Instruction {
    Op op = Op::Halt;
    std::int32_t operand = 0;
};

struct FunctionInfo {
    std::string name;
    std::vector<std::string> params;
    std::int32_t entry = 0;
};

struct ClassInfo {
    std::string name;
    std::string base_name;
    std::vector<std::string> fields;
    std::vector<std::string> methods;
};

struct Chunk {
    std::vector<Instruction> code;
    std::vector<Value> constants;
    std::vector<std::string> names;
    std::vector<FunctionInfo> functions;
    std::vector<ClassInfo> classes;
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
