#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ternet {

enum class TokenType {
    String,
    KeywordPrint,
    ColonColon,
    LParen,
    RParen,
    End
};

struct Token {
    TokenType type;
    std::string text;
    std::size_t line;
    std::size_t column;
};

std::vector<Token> lex(const std::string& source);

struct PrintStatement {
    std::string value;
};

struct Program {
    std::vector<PrintStatement> statements;
};

Program parse(const std::vector<Token>& tokens);

enum class OpCode : std::uint8_t {
    PrintString,
    Halt
};

struct Instruction {
    OpCode op;
    std::string operand;
};

std::vector<Instruction> compile(const Program& program);

class VM {
public:
    void run(const std::vector<Instruction>& code);
};

}
