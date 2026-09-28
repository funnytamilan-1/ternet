#include "ternet.hpp"

namespace ternet {

std::vector<Instruction> compile(const Program& program) {
    std::vector<Instruction> code;
    for (const auto& statement : program.statements) {
        code.push_back({OpCode::PrintString, statement.value});
    }
    code.push_back({OpCode::Halt, ""});
    return code;
}

}
