#include "ternet.hpp"
#include <cctype>
#include <stdexcept>

namespace ternet {

class Parser {
    const std::vector<Token>& tokens;
    std::size_t i = 0;

    Token& c() {
        return const_cast<Token&>(tokens[i]);
    }

    const Token& peek(std::size_t offset = 0) const {
        if (i + offset < tokens.size()) return tokens[i + offset];
        return tokens.back();
    }

    bool is(TokenType k, const char* text = nullptr) const {
        if (i >= tokens.size()) return false;
        return tokens[i].type == k && (!text || tokens[i].text == text);
    }

    bool eat(TokenType k, const char* text = nullptr) {
        if (is(k, text)) {
            ++i;
            return true;
        }
        return false;
    }

    void need(TokenType k, const char* msg) {
        if (!eat(k)) {
            throw RuntimeError(std::string(msg) + " at " +
                               std::to_string(c().pos.line) + ":" + std::to_string(c().pos.column),
                               c().pos, "T1004");
        }
    }

    void need_op(const std::string& op_text, const char* msg) {
        if (is(TokenType::Op, op_text.c_str())) {
            ++i;
            return;
        }
        throw RuntimeError(std::string(msg) + " at " +
                           std::to_string(c().pos.line) + ":" + std::to_string(c().pos.column),
                           c().pos, "T1005");
    }

    void end() {
        if (eat(TokenType::Colon) || eat(TokenType::Semicolon)) {
            return;
        }
        // Optional delimiter if at block boundary or next statement keyword
        if (is(TokenType::RBrace) || is(TokenType::End)) return;
    }

    std::string type_signature() {
        if (is(TokenType::LParen)) {
            // Tuple or Function type: (int, String) or (int) -> void
            std::string res = "(";
            eat(TokenType::LParen);
            bool first = true;
            while (!is(TokenType::RParen) && !is(TokenType::End)) {
                if (!first) {
                    need(TokenType::Comma, "expected ',' between type elements");
                    res += ", ";
                }
                first = false;
                res += type_signature();
            }
            need(TokenType::RParen, "expected ')' after tuple type");
            res += ")";
            if (is(TokenType::Op, "->")) {
                eat(TokenType::Op, "->");
                res += " -> " + type_signature();
            }
            return res;
        }

        if (is(TokenType::Keyword) || is(TokenType::Identifier)) {
            std::string t = c().text;
            ++i;
            // Check for generic type arguments: List<int>, Map<String, int>, Option<int>, Result<T, E>
            if (is(TokenType::Op, "<")) {
                t += "<";
                eat(TokenType::Op, "<");
                bool first = true;
                while (!is(TokenType::Op, ">") && !is(TokenType::End)) {
                    if (!first) {
                        need(TokenType::Comma, "expected ',' between generic type arguments");
                        t += ", ";
                    }
                    first = false;
                    t += type_signature();
                }
                need_op(">", "expected '>' after generic type arguments");
                t += ">";
            }
            // Check for array suffix: int[]
            while (is(TokenType::LBracket) && peek(1).type == TokenType::RBracket) {
                eat(TokenType::LBracket);
                eat(TokenType::RBracket);
                t = "List<" + t + ">";
            }
            return t;
        }

        throw RuntimeError("expected type name at " +
                           std::to_string(c().pos.line) + ":" + std::to_string(c().pos.column),
                           c().pos, "T1006");
    }

    std::vector<StmtPtr> braced() {
        std::vector<StmtPtr> b;
        need(TokenType::LBrace, "expected '{'");
        while (!is(TokenType::RBrace) && !is(TokenType::End)) {
            b.push_back(stmt());
        }
        need(TokenType::RBrace, "expected '}'");
        return b;
    }

    std::vector<StmtPtr> after_header() {
        if (is(TokenType::LBrace)) {
            return braced();
        }
        return {stmt()};
    }

    // Expressions
    ExprPtr expr() {
        return logic_or();
    }

    ExprPtr logic_or() {
        auto left = logic_and();
        while (is(TokenType::Op, "||")) {
            auto op = c().text; ++i;
            auto right = logic_and();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr logic_and() {
        auto left = bitwise_or();
        while (is(TokenType::Op, "&&")) {
            auto op = c().text; ++i;
            auto right = bitwise_or();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr bitwise_or() {
        auto left = bitwise_xor();
        while (is(TokenType::Op, "|")) {
            auto op = c().text; ++i;
            auto right = bitwise_xor();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr bitwise_xor() {
        auto left = bitwise_and();
        while (is(TokenType::Op, "^")) {
            auto op = c().text; ++i;
            auto right = bitwise_and();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr bitwise_and() {
        auto left = equality();
        while (is(TokenType::Op, "&")) {
            auto op = c().text; ++i;
            auto right = equality();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr equality() {
        auto left = comparison();
        while (is(TokenType::Op, "==") || is(TokenType::Op, "!=")) {
            auto op = c().text; ++i;
            auto right = comparison();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr comparison() {
        auto left = shift();
        while (is(TokenType::Op, "<") || is(TokenType::Op, "<=") ||
               is(TokenType::Op, ">") || is(TokenType::Op, ">=")) {
            auto op = c().text; ++i;
            auto right = shift();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr shift() {
        auto left = term();
        while (is(TokenType::Op, "<<") || is(TokenType::Op, ">>")) {
            auto op = c().text; ++i;
            auto right = term();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr term() {
        auto left = factor();
        while (is(TokenType::Op, "+") || is(TokenType::Op, "-")) {
            auto op = c().text; ++i;
            auto right = factor();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr factor() {
        auto left = unary();
        while (is(TokenType::Op, "*") || is(TokenType::Op, "/") || is(TokenType::Op, "%")) {
            auto op = c().text; ++i;
            auto right = unary();
            left = make_bin(op, left, right);
        }
        return left;
    }

    ExprPtr make_bin(const std::string& op, ExprPtr a, ExprPtr b) {
        auto n = std::make_shared<Expr>();
        n->kind = Expr::Binary;
        n->op = op;
        n->left = a;
        n->right = b;
        n->pos = a ? a->pos : SourcePos{1, 1};
        return n;
    }

    ExprPtr unary() {
        if (is(TokenType::Op, "!") || is(TokenType::Op, "-") ||
            is(TokenType::Op, "+") || is(TokenType::Op, "~")) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::Unary;
            n->pos = c().pos;
            n->op = c().text;
            ++i;
            n->right = unary();
            return n;
        }
        return postfix();
    }

    ExprPtr postfix() {
        auto left = primary();
        for (;;) {
            // Function or Method Call: left(args)
            if (eat(TokenType::LParen)) {
                auto n = std::make_shared<Expr>();
                n->kind = Expr::Call;
                n->pos = left->pos;
                n->left = left;
                if (!eat(TokenType::RParen)) {
                    do {
                        n->args.push_back(expr());
                    } while (eat(TokenType::Comma));
                    need(TokenType::RParen, "expected ')' after call arguments");
                }
                left = n;
            }
            // Indexing: left[index]
            else if (eat(TokenType::LBracket)) {
                auto n = std::make_shared<Expr>();
                n->kind = Expr::Index;
                n->pos = left->pos;
                n->left = left;
                n->index = expr();
                need(TokenType::RBracket, "expected ']' after index");
                left = n;
            }
            // Member access or Tuple field access: left.member or left.0
            else if (eat(TokenType::Dot)) {
                auto n = std::make_shared<Expr>();
                n->kind = Expr::Member;
                n->pos = c().pos;
                n->object = left;
                if (is(TokenType::Identifier) || is(TokenType::Keyword) || is(TokenType::Number)) {
                    n->name = c().text;
                    ++i;
                } else {
                    throw RuntimeError("expected member name or index after '.' at " +
                                       std::to_string(c().pos.line) + ":" + std::to_string(c().pos.column),
                                       c().pos, "T1007");
                }
                left = n;
            }
            else {
                break;
            }
        }
        return left;
    }

    ExprPtr primary() {
        SourcePos pos = c().pos;

        // Number literal
        if (is(TokenType::Number)) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::Literal;
            n->pos = pos;
            std::string text = c().text;
            ++i;
            if (text.find('.') != std::string::npos) {
                n->literal = Value(std::stod(text));
            } else if (text.size() > 2 && (text.substr(0, 2) == "0x" || text.substr(0, 2) == "0X")) {
                n->literal = Value(static_cast<std::int64_t>(std::stoll(text, nullptr, 16)));
            } else if (text.size() > 2 && (text.substr(0, 2) == "0b" || text.substr(0, 2) == "0B")) {
                n->literal = Value(static_cast<std::int64_t>(std::stoll(text.substr(2), nullptr, 2)));
            } else {
                n->literal = Value(static_cast<std::int64_t>(std::stoll(text)));
            }
            return n;
        }

        // String literal
        if (is(TokenType::String)) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::Literal;
            n->pos = pos;
            n->literal = Value(c().text);
            ++i;
            return n;
        }

        // Boolean literal
        if (is(TokenType::Keyword, "true") || is(TokenType::Keyword, "false")) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::Literal;
            n->pos = pos;
            n->literal = Value(c().text == "true");
            ++i;
            return n;
        }

        // Null literal
        if (is(TokenType::Keyword, "null")) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::Literal;
            n->pos = pos;
            n->literal = Value();
            ++i;
            return n;
        }

        // This
        if (is(TokenType::Keyword, "this")) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::This;
            n->name = "this";
            n->pos = pos;
            ++i;
            return n;
        }

        // Super
        if (is(TokenType::Keyword, "super")) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::Super;
            n->name = "super";
            n->pos = pos;
            ++i;
            return n;
        }

        // New Class(...) expression
        if (is(TokenType::Keyword, "new")) {
            ++i;
            return primary();
        }

        // Identifiers and keyword-named functions (e.g. print, tnprint, some, none, ok, err, is_some, etc.)
        if (is(TokenType::Identifier) || is(TokenType::Keyword)) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::Variable;
            n->pos = pos;
            n->name = c().text;
            ++i;
            return n;
        }

        // Parentheses or Tuples: (expr) or (a, b, c)
        if (eat(TokenType::LParen)) {
            if (eat(TokenType::RParen)) {
                // Empty tuple ()
                auto n = std::make_shared<Expr>();
                n->kind = Expr::Tuple;
                n->pos = pos;
                return n;
            }
            auto first = expr();
            if (eat(TokenType::Comma)) {
                auto n = std::make_shared<Expr>();
                n->kind = Expr::Tuple;
                n->pos = pos;
                n->args.push_back(first);
                do {
                    n->args.push_back(expr());
                } while (eat(TokenType::Comma));
                need(TokenType::RParen, "expected ')' after tuple elements");
                return n;
            }
            need(TokenType::RParen, "expected ')'");
            return first;
        }

        // Array / List literal: [a, b, c]
        if (eat(TokenType::LBracket)) {
            auto n = std::make_shared<Expr>();
            n->kind = Expr::Array;
            n->pos = pos;
            if (!eat(TokenType::RBracket)) {
                do {
                    n->args.push_back(expr());
                } while (eat(TokenType::Comma));
                need(TokenType::RBracket, "expected ']' after list elements");
            }
            return n;
        }

        throw RuntimeError("expected expression at " +
                           std::to_string(c().pos.line) + ":" + std::to_string(c().pos.column),
                           c().pos, "T1008");
    }

    // Statements
    StmtPtr declaration(bool explicit_type = false, const std::string& type_str = "") {
        auto s = std::make_shared<Stmt>();
        s->kind = Stmt::Let;
        s->pos = c().pos;
        s->mutable_binding = true;
        if (explicit_type) {
            s->type_name = type_str.empty() ? type_signature() : type_str;
        }
        if (!is(TokenType::Identifier)) {
            throw RuntimeError("expected binding name at " +
                               std::to_string(c().pos.line) + ":" + std::to_string(c().pos.column),
                               c().pos, "T1009");
        }
        s->name = c().text;
        ++i;

        // Check if explicit type with colon follows: let x: int = 5
        if (eat(TokenType::Colon)) {
            s->type_name = type_signature();
        }

        if (eat(TokenType::Op, "=")) {
            s->expr = expr();
        }
        end();
        return s;
    }

public:
    Parser(const std::vector<Token>& x) : tokens(x) {}

    StmtPtr stmt() {
        auto s = std::make_shared<Stmt>();
        s->pos = c().pos;

        // import "path.trn" or import module
        if (is(TokenType::Keyword, "import")) {
            ++i;
            s->kind = Stmt::Import;
            if (is(TokenType::String)) {
                s->module_path = c().text;
                ++i;
            } else {
                if (!is(TokenType::Identifier)) throw RuntimeError("expected module name after import", c().pos, "T1010");
                s->module_path = c().text;
                ++i;
                while (eat(TokenType::Dot)) {
                    if (!is(TokenType::Identifier) && !is(TokenType::Keyword))
                        throw RuntimeError("expected module segment after '.'", c().pos, "T1010");
                    s->module_path += "/" + c().text;
                    ++i;
                }
            }
            end();
            return s;
        }

        // webfile "path" { ... }
        if (is(TokenType::Keyword, "webfile")) {
            ++i;
            if (!is(TokenType::String)) throw RuntimeError("expected output path after webfile", c().pos, "T1011");
            s->kind = Stmt::WebFile;
            s->web_path = c().text;
            ++i;
            need(TokenType::LBrace, "expected '{' after webfile path");
            while (!is(TokenType::RBrace) && !is(TokenType::End)) {
                s->web_parts.push_back(expr());
                end();
            }
            need(TokenType::RBrace, "expected '}' after webfile");
            eat(TokenType::Colon);
            return s;
        }

        // let, mut, const variable declarations
        if (is(TokenType::Keyword, "let") || is(TokenType::Keyword, "mut") || is(TokenType::Keyword, "const")) {
            auto kw = c().text;
            ++i;
            s = declaration();
            s->mutable_binding = (kw == "mut" || kw == "let");
            return s;
        }

        // Explicit type variable declaration: int x = 1: or List<int> nums = [1, 2]:
        // Detect if current token is a type keyword followed by an identifier and '=' or ':'
        if (is(TokenType::Keyword, "int") || is(TokenType::Keyword, "str") ||
            is(TokenType::Keyword, "string") || is(TokenType::Keyword, "float") ||
            is(TokenType::Keyword, "bool") || is(TokenType::Keyword, "size") ||
            is(TokenType::Keyword, "char") || is(TokenType::Keyword, "byte") ||
            is(TokenType::Keyword, "uint") || is(TokenType::Keyword, "int8") ||
            is(TokenType::Keyword, "int16") || is(TokenType::Keyword, "int32") ||
            is(TokenType::Keyword, "int64") || is(TokenType::Keyword, "uint8") ||
            is(TokenType::Keyword, "uint16") || is(TokenType::Keyword, "uint32") ||
            is(TokenType::Keyword, "uint64") || is(TokenType::Keyword, "float32") ||
            is(TokenType::Keyword, "float64") || is(TokenType::Keyword, "void") ||
            is(TokenType::Keyword, "List") || is(TokenType::Keyword, "Map") ||
            is(TokenType::Keyword, "Array") || is(TokenType::Keyword, "Option") ||
            is(TokenType::Keyword, "Result")) {
            // Check if next token is identifier
            std::size_t save = i;
            std::string parsed_t = type_signature();
            if (is(TokenType::Identifier) && (peek(1).text == "=" || peek(1).text == ":" || peek(1).type == TokenType::Colon)) {
                return declaration(true, parsed_t);
            }
            i = save;
        }

        // print(...) and tnprint(...)
        if (is(TokenType::Keyword, "print") || is(TokenType::Keyword, "tnprint")) {
            ++i;
            need(TokenType::LParen, "expected '(' after print");
            s->kind = Stmt::Print;
            if (!eat(TokenType::RParen)) {
                do {
                    s->print_args.push_back(expr());
                } while (eat(TokenType::Comma));
                need(TokenType::RParen, "expected ')'");
            }
            end();
            return s;
        }

        // if condition { ... } elif condition { ... } else { ... }
        if (is(TokenType::Keyword, "if")) {
            s->kind = Stmt::If;
            ++i;
            ExprPtr cond;
            if (eat(TokenType::LParen)) {
                cond = expr();
                need(TokenType::RParen, "expected ')' after if condition");
            } else if (eat(TokenType::LBrace)) {
                cond = expr();
                need(TokenType::RBrace, "expected '}' after if condition");
            } else {
                cond = expr();
            }
            eat(TokenType::Semicolon);
            eat(TokenType::Colon);
            s->branches.push_back({cond, after_header()});

            while (eat(TokenType::Keyword, "elif") || eat(TokenType::Keyword, "else if")) {
                ExprPtr cc;
                if (eat(TokenType::LParen)) {
                    cc = expr();
                    need(TokenType::RParen, "expected ')' after elif condition");
                } else if (eat(TokenType::LBrace)) {
                    cc = expr();
                    need(TokenType::RBrace, "expected '}' after elif condition");
                } else {
                    cc = expr();
                }
                eat(TokenType::Semicolon);
                eat(TokenType::Colon);
                s->branches.push_back({cc, after_header()});
            }

            if (eat(TokenType::Keyword, "else")) {
                if (eat(TokenType::LBrace)) {
                    if (eat(TokenType::RBrace)) {
                        eat(TokenType::Semicolon);
                        eat(TokenType::Colon);
                        s->else_body.push_back(stmt());
                    } else {
                        while (!is(TokenType::RBrace) && !is(TokenType::End)) {
                            s->else_body.push_back(stmt());
                        }
                        need(TokenType::RBrace, "expected '}'");
                        eat(TokenType::Semicolon);
                        eat(TokenType::Colon);
                    }
                } else {
                    s->else_body.push_back(stmt());
                }
            }
            return s;
        }

        // while condition { ... }
        if (is(TokenType::Keyword, "while")) {
            s->kind = Stmt::While;
            ++i;
            ExprPtr cond;
            if (eat(TokenType::LParen)) {
                cond = expr();
                need(TokenType::RParen, "expected ')' after while condition");
            } else if (eat(TokenType::LBrace)) {
                cond = expr();
                need(TokenType::RBrace, "expected '}' after while condition");
            } else {
                cond = expr();
            }
            eat(TokenType::Semicolon);
            eat(TokenType::Colon);
            s->expr = cond;
            s->body = is(TokenType::LBrace) ? braced() : std::vector<StmtPtr>{stmt()};
            return s;
        }

        // for var in start..end { ... } or for var in collection { ... }
        if (is(TokenType::Keyword, "for")) {
            s->kind = Stmt::For;
            ++i;
            if (!is(TokenType::Identifier)) throw RuntimeError("expected loop variable", c().pos, "T1012");
            s->name = c().text;
            ++i;
            need(TokenType::Keyword, "expected 'in' in for loop");
            s->for_start = expr();
            if (eat(TokenType::Op, "..")) {
                s->for_end = expr();
                s->for_inclusive = true;
            }
            s->body = braced();
            return s;
        }

        // enum Name { VAL1, VAL2, ... }
        if (is(TokenType::Keyword, "enum")) {
            ++i;
            if (!is(TokenType::Identifier)) throw RuntimeError("expected enum name", c().pos, "T1013");
            s->kind = Stmt::Enum;
            s->name = c().text;
            ++i;
            need(TokenType::LBrace, "expected '{' after enum name");
            while (!is(TokenType::RBrace) && !is(TokenType::End)) {
                if (!is(TokenType::Identifier) && !is(TokenType::Keyword))
                    throw RuntimeError("expected enum value identifier", c().pos, "T1013");
                s->enum_values.push_back(c().text);
                ++i;
                if (!eat(TokenType::Comma)) eat(TokenType::Colon);
            }
            need(TokenType::RBrace, "expected '}' after enum");
            eat(TokenType::Colon);
            return s;
        }

        // match expr { pattern => body, ... }
        if (is(TokenType::Keyword, "match")) {
            ++i;
            s->kind = Stmt::Match;
            s->match_expr = expr();
            need(TokenType::LBrace, "expected '{' after match expression");
            while (!is(TokenType::RBrace) && !is(TokenType::End)) {
                Stmt::MatchCase mc;
                if (is(TokenType::Identifier, "_")) {
                    ++i;
                    mc.is_wildcard = true;
                    need_op("=>", "expected '=>' after match case");
                    mc.body = after_header();
                    s->match_cases.push_back(mc);
                    s->match_default = mc.body;
                } else if (is(TokenType::Keyword, "some") || is(TokenType::Keyword, "ok") || is(TokenType::Keyword, "err")) {
                    mc.pattern_tag = c().text;
                    ++i;
                    need(TokenType::LParen, "expected '(' after pattern tag");
                    if (is(TokenType::Identifier)) {
                        mc.pattern_var = c().text;
                        ++i;
                    }
                    need(TokenType::RParen, "expected ')'");
                    need_op("=>", "expected '=>' after match case");
                    mc.body = after_header();
                    s->match_cases.push_back(mc);
                } else if (is(TokenType::Keyword, "none")) {
                    mc.pattern_tag = "none";
                    ++i;
                    if (eat(TokenType::LParen)) eat(TokenType::RParen);
                    need_op("=>", "expected '=>' after match case");
                    mc.body = after_header();
                    s->match_cases.push_back(mc);
                } else {
                    mc.pattern = expr();
                    need_op("=>", "expected '=>' after match case");
                    mc.body = after_header();
                    s->match_cases.push_back(mc);
                }
            }
            need(TokenType::RBrace, "expected '}' after match");
            eat(TokenType::Colon);
            return s;
        }

        // struct Name { Type field: ... }
        if (is(TokenType::Keyword, "struct")) {
            ++i;
            if (!is(TokenType::Identifier)) throw RuntimeError("expected struct name", c().pos, "T1014");
            s->kind = Stmt::Struct;
            s->name = c().text;
            ++i;
            need(TokenType::LBrace, "expected '{' after struct name");
            while (!is(TokenType::RBrace) && !is(TokenType::End)) {
                if (eat(TokenType::Keyword, "let") || eat(TokenType::Keyword, "mut") || eat(TokenType::Keyword, "const")) {}
                std::string ft = type_signature();
                if (!is(TokenType::Identifier) && !is(TokenType::Keyword))
                    throw RuntimeError("expected field name", c().pos, "T1014");
                std::string fn = c().text;
                ++i;
                s->field_types.push_back(ft);
                s->fields.push_back(fn);
                end();
            }
            need(TokenType::RBrace, "expected '}' after struct");
            eat(TokenType::Colon);
            return s;
        }

        // trait Name { fn method() ... } or interface Name { ... }
        if (is(TokenType::Keyword, "trait") || is(TokenType::Keyword, "interface")) {
            ++i;
            if (!is(TokenType::Identifier)) throw RuntimeError("expected trait name", c().pos, "T1015");
            s->kind = Stmt::Trait;
            s->name = c().text;
            ++i;
            need(TokenType::LBrace, "expected '{' after trait name");
            while (!is(TokenType::RBrace) && !is(TokenType::End)) {
                s->methods.push_back(stmt());
            }
            need(TokenType::RBrace, "expected '}' after trait");
            eat(TokenType::Colon);
            return s;
        }

        // class Name [extends Base] [implements Trait1, Trait2] { ... }
        if (is(TokenType::Keyword, "class")) {
            ++i;
            if (!is(TokenType::Identifier)) throw RuntimeError("expected class name", c().pos, "T1016");
            s->kind = Stmt::Class;
            s->name = c().text;
            ++i;

            // Generic parameters: class Box<T>
            if (eat(TokenType::Op, "<")) {
                do {
                    if (!is(TokenType::Identifier)) throw RuntimeError("expected generic type parameter", c().pos, "T1016");
                    s->generic_params.push_back(c().text);
                    ++i;
                } while (eat(TokenType::Comma));
                need_op(">", "expected '>' after generic parameters");
            }

            // Inheritance: extends Base
            if (eat(TokenType::Keyword, "extends")) {
                if (!is(TokenType::Identifier)) throw RuntimeError("expected base class name", c().pos, "T1017");
                s->base_name = c().text;
                ++i;
            }

            // Interfaces/Traits: implements Trait1, Trait2
            if (eat(TokenType::Keyword, "implements")) {
                do {
                    if (!is(TokenType::Identifier)) throw RuntimeError("expected trait name", c().pos, "T1018");
                    s->traits.push_back(c().text);
                    ++i;
                } while (eat(TokenType::Comma));
            }

            need(TokenType::LBrace, "expected '{' after class header");
            while (!is(TokenType::RBrace) && !is(TokenType::End)) {
                // Member method or field
                s->methods.push_back(stmt());
            }
            need(TokenType::RBrace, "expected '}' after class");
            eat(TokenType::Colon);
            return s;
        }

        // Modifiers for functions/methods: virtual, override, static, abstract
        bool is_virt = false, is_over = false, is_stat = false, is_abst = false;
        while (is(TokenType::Keyword, "virtual") || is(TokenType::Keyword, "override") ||
               is(TokenType::Keyword, "static") || is(TokenType::Keyword, "abstract") ||
               is(TokenType::Keyword, "public") || is(TokenType::Keyword, "private") ||
               is(TokenType::Keyword, "protected")) {
            if (c().text == "virtual") is_virt = true;
            if (c().text == "override") is_over = true;
            if (c().text == "static") is_stat = true;
            if (c().text == "abstract") is_abst = true;
            ++i;
        }

        // fn name(params) -> ReturnType { ... } or lit name(params) { ... }
        if (is(TokenType::Keyword, "fn") || is(TokenType::Keyword, "lit")) {
            ++i;
            if (!is(TokenType::Identifier) && !is(TokenType::Keyword, "init"))
                throw RuntimeError("expected function name", c().pos, "T1019");
            s->kind = Stmt::Function;
            s->name = c().text;
            s->is_virtual = is_virt;
            s->is_override = is_over;
            s->is_static = is_stat;
            s->is_abstract = is_abst;
            ++i;

            // Generic function parameters: fn id<T>(v: T) -> T
            if (eat(TokenType::Op, "<")) {
                do {
                    if (!is(TokenType::Identifier)) throw RuntimeError("expected generic type parameter", c().pos, "T1019");
                    s->generic_params.push_back(c().text);
                    ++i;
                } while (eat(TokenType::Comma));
                need_op(">", "expected '>' after generic parameters");
            }

            need(TokenType::LParen, "expected '(' after function name");
            if (!eat(TokenType::RParen)) {
                do {
                    std::string param_type;
                    std::string param_name;

                    // Syntax 1: param: Type
                    // Syntax 2: Type param
                    // Syntax 3: param
                    if (is(TokenType::Identifier)) {
                        auto save_pos = i;
                        std::string first_ident = c().text;
                        ++i;
                        if (eat(TokenType::Colon)) {
                            param_name = first_ident;
                            param_type = type_signature();
                        } else if (is(TokenType::Identifier)) {
                            param_type = first_ident;
                            param_name = c().text;
                            ++i;
                        } else {
                            param_name = first_ident;
                        }
                    } else if (is(TokenType::Keyword)) {
                        param_type = type_signature();
                        if (!is(TokenType::Identifier)) throw RuntimeError("expected parameter name", c().pos, "T1020");
                        param_name = c().text;
                        ++i;
                    }
                    s->params.push_back(param_name);
                    s->param_types.push_back(param_type);
                } while (eat(TokenType::Comma));
                need(TokenType::RParen, "expected ')' after parameters");
            }

            if (eat(TokenType::Op, "->") || eat(TokenType::Colon)) {
                s->return_type = type_signature();
            }

            if (is_abst || is(TokenType::Colon) || is(TokenType::Semicolon)) {
                end();
            } else if (is(TokenType::LBrace)) {
                s->function_body = braced();
                eat(TokenType::Colon);
            }
            return s;
        }

        // throw expr
        if (is(TokenType::Keyword, "throw")) {
            ++i;
            s->kind = Stmt::Throw;
            s->expr = expr();
            end();
            return s;
        }

        // try { ... } catch (e) { ... } finally { ... }
        if (is(TokenType::Keyword, "try")) {
            ++i;
            s->kind = Stmt::Try;
            s->body = braced();
            if (eat(TokenType::Keyword, "catch")) {
                if (eat(TokenType::LParen)) {
                    if (is(TokenType::Identifier)) {
                        s->catch_name = c().text;
                        ++i;
                        if (eat(TokenType::Colon)) s->catch_type = type_signature();
                    }
                    need(TokenType::RParen, "expected ')' after catch parameter");
                } else if (is(TokenType::Identifier)) {
                    s->catch_name = c().text;
                    ++i;
                }
                s->catch_body = braced();
            }
            if (eat(TokenType::Keyword, "finally")) {
                s->finally_body = braced();
            }
            if (s->catch_body.empty() && s->finally_body.empty()) {
                throw RuntimeError("try requires catch or finally at " +
                                   std::to_string(s->pos.line) + ":" + std::to_string(s->pos.column),
                                   s->pos, "T1021");
            }
            return s;
        }

        // return expr
        if (is(TokenType::Keyword, "return")) {
            ++i;
            s->kind = Stmt::Return;
            if (!is(TokenType::Colon) && !is(TokenType::Semicolon) && !is(TokenType::RBrace) && !is(TokenType::End)) {
                s->expr = expr();
            }
            end();
            return s;
        }

        // break and continue
        if (is(TokenType::Keyword, "break")) {
            ++i;
            s->kind = Stmt::Break;
            end();
            return s;
        }
        if (is(TokenType::Keyword, "continue")) {
            ++i;
            s->kind = Stmt::Continue;
            end();
            return s;
        }

        // Assignment: lhs = expr or lhs += expr
        // Check if an assignment
        std::size_t save = i;
        ExprPtr lhs = postfix();
        if (is(TokenType::Op, "=") || is(TokenType::Op, "+=") || is(TokenType::Op, "-=") ||
            is(TokenType::Op, "*=") || is(TokenType::Op, "/=") || is(TokenType::Op, "%=")) {
            std::string op = c().text;
            ++i;
            s->kind = Stmt::Assign;
            s->target = lhs;
            s->name = (lhs->kind == Expr::Variable) ? lhs->name : "";
            auto rhs = expr();
            if (op == "=") {
                s->expr = rhs;
            } else {
                std::string base_op = op.substr(0, 1);
                s->expr = make_bin(base_op, lhs, rhs);
            }
            end();
            return s;
        }

        // Expression statement
        s->kind = Stmt::ExprStmt;
        s->expr = lhs;
        end();
        return s;
    }

    Program parse() {
        Program p;
        while (!is(TokenType::End)) {
            p.statements.push_back(stmt());
        }
        return p;
    }
};

Program parse(const std::vector<Token>& tokens) {
    return Parser(tokens).parse();
}

} // namespace ternet
