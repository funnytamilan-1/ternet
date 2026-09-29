#include "ternet.hpp"
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace ternet {

std::vector<Token> lex(const std::string& source) {
    std::vector<Token> tokens;
    std::size_t i = 0;
    std::size_t line = 1;
    std::size_t col = 1;

    auto adv = [&]() {
        if (i < source.size()) {
            if (source[i] == '\n') {
                ++line;
                col = 1;
            } else {
                ++col;
            }
            ++i;
        }
    };

    static const std::unordered_set<std::string> keywords = {
        "let", "mut", "const", "fn", "lit", "return", "if", "elif", "else", "while", "for", "in",
        "break", "continue", "true", "false", "null", "import", "from", "as", "struct", "class",
        "enum", "trait", "interface", "impl", "implements", "extends", "public", "private",
        "protected", "virtual", "override", "abstract", "final", "static", "this", "super",
        "match", "async", "await", "spawn", "task", "channel", "throw", "try", "catch", "finally",
        "comptime", "tnprint", "print", "webfile", "int", "str", "string", "float", "bool",
        "void", "size", "char", "byte", "uint", "int8", "int16", "int32", "int64", "uint8",
        "uint16", "uint32", "uint64", "float32", "float64", "never", "List", "Array", "Map",
        "Tuple", "Option", "Result", "some", "none", "ok", "err", "is_some", "is_none", "is_ok",
        "is_err", "unwrap", "unwrap_or", "new", "typeof", "is", "with", "yield", "lambda",
        "case", "where", "package", "export", "generic", "macro", "attribute", "union", "extern", "auto"
    };

    while (i < source.size()) {
        char ch = source[i];

        // Whitespace
        if (std::isspace(static_cast<unsigned char>(ch))) {
            adv();
            continue;
        }

        // Single-line comment
        if (ch == '/' && i + 1 < source.size() && source[i + 1] == '/') {
            while (i < source.size() && source[i] != '\n') {
                adv();
            }
            continue;
        }

        // Multi-line comment
        if (ch == '/' && i + 1 < source.size() && source[i + 1] == '*') {
            std::size_t start_line = line;
            std::size_t start_col = col;
            adv(); // consume '/'
            adv(); // consume '*'
            bool closed = false;
            while (i < source.size()) {
                if (source[i] == '*' && i + 1 < source.size() && source[i + 1] == '/') {
                    adv(); // consume '*'
                    adv(); // consume '/'
                    closed = true;
                    break;
                }
                adv();
            }
            if (!closed) {
                throw RuntimeError("unterminated block comment at " +
                                   std::to_string(start_line) + ":" + std::to_string(start_col),
                                   {start_line, start_col}, "T1001");
            }
            continue;
        }

        std::size_t token_line = line;
        std::size_t token_col = col;

        // Identifier or Keyword
        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            std::string ident;
            while (i < source.size() &&
                   (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_')) {
                ident += source[i];
                adv();
            }
            TokenType type = (keywords.count(ident) > 0) ? TokenType::Keyword : TokenType::Identifier;
            tokens.push_back({type, ident, {token_line, token_col}});
            continue;
        }

        // Number literal (integers, hex, bin, floats)
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            std::string num_str;
            if (ch == '0' && i + 1 < source.size() && (source[i + 1] == 'x' || source[i + 1] == 'X')) {
                num_str += source[i]; adv();
                num_str += source[i]; adv();
                while (i < source.size() && std::isxdigit(static_cast<unsigned char>(source[i]))) {
                    num_str += source[i];
                    adv();
                }
                tokens.push_back({TokenType::Number, num_str, {token_line, token_col}});
                continue;
            }
            if (ch == '0' && i + 1 < source.size() && (source[i + 1] == 'b' || source[i + 1] == 'B')) {
                num_str += source[i]; adv();
                num_str += source[i]; adv();
                while (i < source.size() && (source[i] == '0' || source[i] == '1')) {
                    num_str += source[i];
                    adv();
                }
                tokens.push_back({TokenType::Number, num_str, {token_line, token_col}});
                continue;
            }

            while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i]))) {
                num_str += source[i];
                adv();
            }

            // Check if followed by range operator '..'
            if (i < source.size() && source[i] == '.' && i + 1 < source.size() && source[i + 1] == '.') {
                tokens.push_back({TokenType::Number, num_str, {token_line, token_col}});
                std::size_t op_line = line;
                std::size_t op_col = col;
                adv(); // '.'
                adv(); // '.'
                tokens.push_back({TokenType::Op, "..", {op_line, op_col}});
                continue;
            }

            // Decimal float: . followed by digit
            if (i < source.size() && source[i] == '.' && i + 1 < source.size() &&
                std::isdigit(static_cast<unsigned char>(source[i + 1]))) {
                num_str += source[i];
                adv();
                while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i]))) {
                    num_str += source[i];
                    adv();
                }
            }

            tokens.push_back({TokenType::Number, num_str, {token_line, token_col}});
            continue;
        }

        // String literal ("..." or '...')
        if (ch == '"' || ch == '\'') {
            char quote = ch;
            adv();
            std::string str_val;
            bool closed = false;
            while (i < source.size()) {
                if (source[i] == '\\') {
                    adv();
                    if (i >= source.size()) break;
                    char esc = source[i];
                    adv();
                    if (esc == 'n') str_val += '\n';
                    else if (esc == 't') str_val += '\t';
                    else if (esc == 'r') str_val += '\r';
                    else if (esc == '\\') str_val += '\\';
                    else if (esc == '"') str_val += '"';
                    else if (esc == '\'') str_val += '\'';
                    else if (esc == '0') str_val += '\0';
                    else str_val += esc;
                } else if (source[i] == quote) {
                    adv();
                    closed = true;
                    break;
                } else {
                    str_val += source[i];
                    adv();
                }
            }
            if (!closed) {
                throw RuntimeError("unterminated string at " +
                                   std::to_string(token_line) + ":" + std::to_string(token_col),
                                   {token_line, token_col}, "T1002");
            }
            tokens.push_back({TokenType::String, str_val, {token_line, token_col}});
            continue;
        }

        // Delimiters
        if (ch == '(') { tokens.push_back({TokenType::LParen, "(", {token_line, token_col}}); adv(); continue; }
        if (ch == ')') { tokens.push_back({TokenType::RParen, ")", {token_line, token_col}}); adv(); continue; }
        if (ch == '{') { tokens.push_back({TokenType::LBrace, "{", {token_line, token_col}}); adv(); continue; }
        if (ch == '}') { tokens.push_back({TokenType::RBrace, "}", {token_line, token_col}}); adv(); continue; }
        if (ch == '[') { tokens.push_back({TokenType::LBracket, "[", {token_line, token_col}}); adv(); continue; }
        if (ch == ']') { tokens.push_back({TokenType::RBracket, "]", {token_line, token_col}}); adv(); continue; }
        if (ch == ',') { tokens.push_back({TokenType::Comma, ",", {token_line, token_col}}); adv(); continue; }
        if (ch == ';') { tokens.push_back({TokenType::Semicolon, ";", {token_line, token_col}}); adv(); continue; }

        // Multi-character operators
        std::string two = (i + 1 < source.size()) ? source.substr(i, 2) : "";
        if (two == "==" || two == "!=" || two == ">=" || two == "<=" ||
            two == "&&" || two == "||" || two == "+=" || two == "-=" ||
            two == "*=" || two == "/=" || two == "%=" || two == "=>" ||
            two == "->" || two == ".." || two == "::" || two == "<<" || two == ">>") {
            tokens.push_back({TokenType::Op, two, {token_line, token_col}});
            adv();
            adv();
            continue;
        }

        // Single-character ':' (colon can be colon or token)
        if (ch == ':') {
            tokens.push_back({TokenType::Colon, ":", {token_line, token_col}});
            adv();
            continue;
        }

        // Single-character '.'
        if (ch == '.') {
            tokens.push_back({TokenType::Dot, ".", {token_line, token_col}});
            adv();
            continue;
        }

        // Single-character operators
        if (std::string("+-*/%!=<>!&|^~?").find(ch) != std::string::npos) {
            tokens.push_back({TokenType::Op, std::string(1, ch), {token_line, token_col}});
            adv();
            continue;
        }

        throw RuntimeError("unexpected character '" + std::string(1, ch) + "' at " +
                           std::to_string(token_line) + ":" + std::to_string(token_col),
                           {token_line, token_col}, "T1003");
    }

    tokens.push_back({TokenType::End, "", {line, col}});
    return tokens;
}

} // namespace ternet
