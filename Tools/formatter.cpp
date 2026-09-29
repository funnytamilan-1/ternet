#include "ternet.hpp"

#include <string>

namespace ternet {

std::string format_source(const std::string& source) {
    auto tokens = lex(source);
    std::string out;
    int indent = 0;
    auto emit_indent = [&]() {
        for (int k = 0; k < indent; ++k) out += "    ";
    };

    bool line_start = true;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& t = tokens[i];
        if (t.type == TokenType::End) break;

        if (t.type == TokenType::RBrace) {
            if (indent > 0) --indent;
            if (!line_start) out += "\n";
            emit_indent();
            out += "}";
            out += "\n";
            line_start = true;
            continue;
        }

        if (line_start) {
            emit_indent();
            line_start = false;
        }

        if (t.type == TokenType::String) {
            out += "\"" + t.text + "\"";
        } else if (t.type == TokenType::LBrace) {
            out += " {";
            out += "\n";
            ++indent;
            line_start = true;
            continue;
        } else if (t.type == TokenType::Comma) {
            out += ", ";
        } else if (t.type == TokenType::Colon) {
            if (i + 1 < tokens.size() && tokens[i + 1].type != TokenType::End &&
                tokens[i + 1].type != TokenType::RBrace && tokens[i + 1].type != TokenType::LBrace) {
                out += ":\n";
                line_start = true;
            } else {
                out += ": ";
            }
        } else if (t.type == TokenType::Semicolon) {
            out += ";\n";
            line_start = true;
        } else if (t.type == TokenType::Op) {
            if (t.text == ".." || t.text == "!" || t.text == "~") {
                out += t.text;
            } else {
                out += " " + t.text + " ";
            }
        } else {
            if (!out.empty() && out.back() != ' ' && out.back() != '\n' &&
                out.back() != '(' && out.back() != '[' && out.back() != '{' && out.back() != '.') {
                out += " ";
            }
            out += t.text;
        }
    }

    if (!out.empty() && out.back() != '\n') out += "\n";
    return out;
}

} // namespace ternet
