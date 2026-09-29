#include "ternet.hpp"
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace ternet {

std::vector<Token> lex(const std::string& source) {
    std::vector<Token> tokens;
    std::size_t i = 0;
    std::size_t line = 1;
    std::size_t column = 1;

    auto advance = [&]() {
        if (i >= source.size()) return;
        if (source[i] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
        ++i;
    };

    auto add = [&](TokenType type, const std::string& text, SourcePos pos) {
        tokens.push_back({type, text, pos});
    };

    static const std::unordered_set<std::string> keywords = {
        "let", "mut", "const", "fn", "lit", "return", "if", "elif", "else",
        "while", "for", "in", "break", "continue", "true", "false", "null",
        "import", "from", "as", "struct", "class", "trait", "impl", "match",
        "async", "await", "throw", "try", "catch", "finally", "comptime",
        "tnprint", "webfile", "int", "str", "float", "bool", "void", "size"
    };

    while (i < source.size()) {
        const char ch = source[i];

        if (std::isspace(static_cast<unsigned char>(ch))) {
            advance();
            continue;
        }

        // Line comment.
        if (ch == '/' && i + 1 < source.size() && source[i + 1] == '/') {
            while (i < source.size() && source[i] != '\n') advance();
            continue;
        }

        // Block comment with proper unterminated-comment diagnostics.
        if (ch == '/' && i + 1 < source.size() && source[i + 1] == '*') {
            const SourcePos start{line, column};
            advance();
            advance();
            bool closed = false;
            while (i < source.size()) {
                if (source[i] == '*' && i + 1 < source.size() && source[i + 1] == '/') {
                    advance();
                    advance();
                    closed = true;
                    break;
                }
                advance();
            }
            if (!closed) {
                throw RuntimeError("unterminated block comment at " +
                                   std::to_string(start.line) + ":" +
                                   std::to_string(start.column));
            }
            continue;
        }

        const SourcePos start{line, column};

        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            std::string text;
            while (i < source.size() &&
                   (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_')) {
                text += source[i];
                advance();
            }
            add(keywords.count(text) ? TokenType::Keyword : TokenType::Identifier, text, start);
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(ch))) {
            std::string text;
            while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i]))) {
                text += source[i];
                advance();
            }

            if (i < source.size() && source[i] == '.' &&
                i + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[i + 1]))) {
                text += source[i];
                advance();
                while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i]))) {
                    text += source[i];
                    advance();
                }
            }

            // Reject malformed numeric forms instead of silently splitting them.
            if (i < source.size() && (std::isalpha(static_cast<unsigned char>(source[i])) || source[i] == '_')) {
                throw RuntimeError("invalid numeric literal at " +
                                   std::to_string(start.line) + ":" +
                                   std::to_string(start.column));
            }

            add(TokenType::Number, text, start);
            continue;
        }

        if (ch == '"') {
            advance();
            std::string text;
            bool closed = false;

            while (i < source.size()) {
                if (source[i] == '"') {
                    advance();
                    closed = true;
                    break;
                }

                if (source[i] == '\n') {
                    throw RuntimeError("unterminated string at " +
                                       std::to_string(start.line) + ":" +
                                       std::to_string(start.column));
                }

                if (source[i] == '\\') {
                    advance();
                    if (i >= source.size()) break;

                    const char escaped = source[i];
                    advance();
                    switch (escaped) {
                        case 'n': text += '\n'; break;
                        case 't': text += '\t'; break;
                        case 'r': text += '\r'; break;
                        case '\\': text += '\\'; break;
                        case '"': text += '"'; break;
                        case '0': text += '\0'; break;
                        default:
                            throw RuntimeError("unknown escape sequence \\" +
                                               std::string(1, escaped) + " at " +
                                               std::to_string(line) + ":" +
                                               std::to_string(column - 1));
                    }
                    continue;
                }

                text += source[i];
                advance();
            }

            if (!closed) {
                throw RuntimeError("unterminated string at " +
                                   std::to_string(start.line) + ":" +
                                   std::to_string(start.column));
            }
            add(TokenType::String, text, start);
            continue;
        }

        TokenType type;
        std::string text(1, ch);

        switch (ch) {
            case '(': type = TokenType::LParen; break;
            case ')': type = TokenType::RParen; break;
            case '{': type = TokenType::LBrace; break;
            case '}': type = TokenType::RBrace; break;
            case '[': type = TokenType::LBracket; break;
            case ']': type = TokenType::RBracket; break;
            case ',': type = TokenType::Comma; break;
            case ':': type = TokenType::Colon; break;
            case ';': type = TokenType::Semicolon; break;
            case '.': type = TokenType::Dot; break;
            default: {
                type = TokenType::Op;
                const std::string two = i + 1 < source.size() ? source.substr(i, 2) : "";
                static const std::unordered_set<std::string> two_char_ops = {
                    "==", "!=", ">=", "<=", "&&", "||", "+=", "-=", "*=", "/=", "=>", ".."
                };
                if (two_char_ops.count(two)) {
                    text = two;
                    advance();
                    advance();
                    add(type, text, start);
                    continue;
                }
                if (std::string("+-*/%!=<>=").find(ch) == std::string::npos) {
                    throw RuntimeError("unexpected character '" + text + "' at " +
                                       std::to_string(start.line) + ":" +
                                       std::to_string(start.column));
                }
                break;
            }
        }

        add(type, text, start);
        advance();
    }

    tokens.push_back({TokenType::End, "", {line, column}});
    return tokens;
}

} // namespace ternet
