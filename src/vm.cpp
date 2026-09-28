#include "ternet.hpp"

#include <iostream>

namespace ternet {

void VM::run(const std::vector<Instruction>& code) {
    for (const auto& instruction : code) {
        switch (instruction.op) {
        case OpCode::PrintString:
            std::cout << instruction.operand << '\n';
            break;
        case OpCode::Halt:
            return;
        }
    }
}

}
