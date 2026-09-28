#include "ternet.hpp"

#include <stdexcept>

namespace ternet {

Program parse(const std::vector<Token>& tokens) {
    Program program;
    std::size_t i = 0;

    auto expect = [&](TokenType type, const char* message) {
        if (tokens[i].type != type) {
            throw std::runtime_error(
                std::string(message) + " at line " +
                std::to_string(tokens[i].line));
        }
    };

    while (tokens[i].type != TokenType::End) {
        expect(TokenType::KeywordPrint, "Expected 'tnprint'");
        ++i;
        expect(TokenType::LParen, "Expected '(' after tnprint");
        ++i;
        expect(TokenType::String, "Expected a string inside tnprint");
        program.statements.push_back({tokens[i].text});
        ++i;
        expect(TokenType::RParen, "Expected ')'");
        ++i;
        expect(TokenType::ColonColon, "Expected '::' after statement");
        ++i;
    }

    return program;
}

}
