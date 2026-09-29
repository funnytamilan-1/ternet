#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ternet {

struct SourcePos {
    std::size_t line = 1;
    std::size_t column = 1;
};

enum class TokenType {
    Identifier,
    Number,
    String,
    Keyword,
    Op,
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,
    Colon,
    Semicolon,
    Dot,
    End
};

struct Token {
    TokenType type;
    std::string text;
    SourcePos pos;
};

std::vector<Token> lex(const std::string& source);

struct Value {
    using Array = std::vector<Value>;
    struct Tuple {
        std::vector<Value> items;
        bool operator==(const Tuple& other) const {
            if (items.size() != other.items.size()) return false;
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (items[i].str() != other.items[i].str()) return false;
            }
            return true;
        }
    };
    using Object = std::unordered_map<std::string, Value>;

    std::variant<std::monostate, bool, std::int64_t, double, std::string, Array, Tuple, Object> data;

    Value() = default;
    template <class T>
    Value(T v) : data(std::move(v)) {}

    bool truthy() const;
    std::string str() const;
    std::string type_name() const;
};

struct Expr;
using ExprPtr = std::shared_ptr<Expr>;

struct Expr {
    enum Kind {
        Literal,
        Variable,
        Unary,
        Binary,
        Call,
        Array,
        Tuple,
        Index,
        Member,
        This,
        Super
    } kind = Literal;

    SourcePos pos;
    Value literal;
    std::string name, op;
    ExprPtr left, right;
    std::vector<ExprPtr> args;
    ExprPtr index;
    ExprPtr object;
    std::vector<std::string> type_args;
};

struct Stmt;
using StmtPtr = std::shared_ptr<Stmt>;

struct Stmt {
    enum Kind {
        ExprStmt,
        Let,
        Assign,
        Print,
        If,
        While,
        For,
        Struct,
        Enum,
        Class,
        Trait,
        Match,
        Import,
        Function,
        Return,
        Break,
        Continue,
        Throw,
        Try,
        WebFile,
        Block
    } kind = ExprStmt;

    SourcePos pos;
    std::string name, type_name, return_type, module_path, base_name;
    bool mutable_binding = true;
    bool is_virtual = false;
    bool is_override = false;
    bool is_static = false;
    bool is_abstract = false;

    ExprPtr expr, for_start, for_end, target;
    bool for_inclusive = true;

    std::vector<ExprPtr> print_args, web_parts;
    std::vector<StmtPtr> body;

    std::vector<std::pair<ExprPtr, std::vector<StmtPtr>>> branches;
    std::vector<StmtPtr> else_body;

    ExprPtr match_expr;
    struct MatchCase {
        ExprPtr pattern;
        std::string pattern_tag;      // e.g. "some", "ok", "err", "none"
        std::string pattern_var;      // variable captured in pattern, e.g. "x" in some(x)
        bool is_wildcard = false;
        std::vector<StmtPtr> body;
    };
    std::vector<MatchCase> match_cases;
    std::vector<StmtPtr> match_default;

    std::vector<std::string> fields, field_types, enum_values, traits, generic_params;
    std::vector<std::string> params, param_types;
    std::vector<StmtPtr> function_body, methods;

    std::string catch_name, catch_type, web_path;
    std::vector<StmtPtr> catch_body, finally_body;
};

struct Program {
    std::vector<StmtPtr> statements;
};

Program parse(const std::vector<Token>& tokens);

class RuntimeError : public std::runtime_error {
public:
    SourcePos pos{1, 1};
    std::string code = "E1000";
    RuntimeError(const std::string& msg, SourcePos p = {1, 1}, const std::string& c = "E1000")
        : std::runtime_error(msg), pos(p), code(c) {}
};

class Interpreter {
public:
    void run(const Program& program);
    Value eval_expr(const ExprPtr& expr);

private:
    struct Binding {
        Value value;
        bool mutable_binding = true;
    };

    struct Function {
        std::vector<std::string> params;
        std::vector<StmtPtr> body;
        bool is_static = false;
        bool is_virtual = false;
        bool is_override = false;
    };

    struct ClassDef {
        std::string name;
        std::string base_name;
        std::vector<std::string> traits;
        std::vector<std::string> fields;
        std::vector<std::string> field_types;
        std::unordered_map<std::string, Function> methods;
        std::unordered_map<std::string, Function> static_methods;
    };

    struct TraitDef {
        std::string name;
        std::unordered_map<std::string, Function> methods;
    };

    struct StructDef {
        std::vector<std::string> fields;
        std::vector<std::string> field_types;
    };

    struct EnumDef {
        std::vector<std::string> values;
    };

    std::vector<std::unordered_map<std::string, Binding>> scopes;
    std::unordered_map<std::string, Function> functions;
    std::unordered_map<std::string, StructDef> structs;
    std::unordered_map<std::string, EnumDef> enums;
    std::unordered_map<std::string, ClassDef> classes;
    std::unordered_map<std::string, TraitDef> traits_map;

    bool returning = false, breaking = false, continuing = false;
    Value return_value;

    Value eval(const ExprPtr& e);
    void exec(const StmtPtr& s);
    void exec_all(const std::vector<StmtPtr>& body);

    Binding* find(const std::string& name);
    static Value binary(const std::string& op, const Value& a, const Value& b);
    Value call_method(const ClassDef& cls, Value::Object& obj, const std::string& method_name, const std::vector<Value>& args);
    Value instantiate_class(const ClassDef& cls, const std::vector<Value>& args);
};

// Package manager commands
int command_init(const std::string& dir);
int command_package(const std::string& dir);
int command_add(const std::string& dir, const std::string& name, const std::string& version);
int command_install(const std::string& dir);
int command_remove(const std::string& dir, const std::string& name);
int command_list(const std::string& dir);

// Tooling: Format, Lint, REPL
std::string format_source(const std::string& source);
std::vector<std::string> lint_source(const std::string& source, const std::string& filename);
int run_repl();

} // namespace ternet
