#include "ternet.hpp"

#include <cctype>
#include <stdexcept>

namespace ternet {

std::vector<Token> lex(const std::string& source) {
    std::vector<Token> tokens;
    std::size_t i = 0;
    std::size_t line = 1;
    std::size_t column = 1;

    auto advance = [&]() {
        if (source[i] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
        ++i;
    };

    while (i < source.size()) {
        const char c = source[i];

        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
            continue;
        }

        const auto start_line = line;
        const auto start_column = column;

        if (c == ':' && i + 1 < source.size() && source[i + 1] == ':') {
            tokens.push_back({TokenType::ColonColon, "::", line, column});
            advance();
            advance();
            continue;
        }

        if (c == '(') {
            tokens.push_back({TokenType::LParen, "(", line, column});
            advance();
            continue;
        }

        if (c == ')') {
            tokens.push_back({TokenType::RParen, ")", line, column});
            advance();
            continue;
        }

        if (c == '"') {
            advance();
            std::string value;

            while (i < source.size() && source[i] != '"') {
                if (source[i] == '\\' && i + 1 < source.size()) {
                    advance();
                    const char escaped = source[i];
                    if (escaped == 'n') value += '\n';
                    else if (escaped == 't') value += '\t';
                    else value += escaped;
                    advance();
                } else {
                    value += source[i];
                    advance();
                }
            }

            if (i >= source.size()) {
                throw std::runtime_error("Unterminated string at line " +
                    std::to_string(start_line));
            }

            advance();
            tokens.push_back({TokenType::String, value, start_line, start_column});
            continue;
        }

        if (source.compare(i, 7, "tnprint") == 0 &&
            (i + 7 == source.size() ||
             !std::isalnum(static_cast<unsigned char>(source[i + 7])))) {
            tokens.push_back({TokenType::KeywordPrint, "tnprint", line, column});
            for (int n = 0; n < 7; ++n) advance();
            continue;
        }

        throw std::runtime_error(
            "Unexpected character '" + std::string(1, c) +
            "' at line " + std::to_string(line) +
            ", column " + std::to_string(column));
    }

    tokens.push_back({TokenType::End, "", line, column});
    return tokens;
}

}
