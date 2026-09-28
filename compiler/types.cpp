#include "types.hpp"
#include <unordered_map>
#include <utility>

namespace ternet::types {

std::string Type::name() const {
    switch (kind) {
    case Kind::Null: return "null";
    case Kind::Bool: return "bool";
    case Kind::Int: return "int";
    case Kind::Float: return "float";
    case Kind::String: return "str";
    case Kind::Array: return "array";
    case Kind::Void: return "void";
    case Kind::User: return custom.empty() ? "user" : custom;
    default: return "unknown";
    }
}

namespace {

struct FunctionSig {
    std::vector<Type> params;
    Type result;
};

struct StructSig {
    std::unordered_map<std::string, Type> fields;
};

struct Checker {
    std::vector<std::unordered_map<std::string, Type>> scopes;
    std::unordered_map<std::string, FunctionSig> functions;
    std::unordered_map<std::string, StructSig> structs;
    std::unordered_map<std::string, std::vector<std::string>> enums;

    Type find(const std::string& n) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto p = it->find(n);
            if (p != it->end()) return p->second;
        }
        throw CheckError("T2001: undefined name '" + n + "'");
    }

    static bool same(Type a, Type b) {
        if (a.kind == Kind::Unknown || b.kind == Kind::Unknown) return true;
        if (a.kind != b.kind) return false;
        return a.kind != Kind::User || a.custom == b.custom;
    }

    static bool numeric(Type t) {
        return t.kind == Kind::Int || t.kind == Kind::Float || t.kind == Kind::Unknown;
    }

    static Type merge_numeric(Type a, Type b) {
        if (a.kind == Kind::Float || b.kind == Kind::Float) return {Kind::Float, {}};
        if (a.kind == Kind::Int && b.kind == Kind::Int) return {Kind::Int, {}};
        return {Kind::Unknown, {}};
    }

    Type parse_type(const std::string& n) const {
        if (n == "int" || n == "size") return {Kind::Int, {}};
        if (n == "str") return {Kind::String, {}};
        if (n == "float") return {Kind::Float, {}};
        if (n == "bool") return {Kind::Bool, {}};
        if (n == "void") return {Kind::Void, {}};
        if (n == "null") return {Kind::Null, {}};
        if (structs.count(n)) return {Kind::User, n};
        if (enums.count(n)) return {Kind::User, n};
        throw CheckError("T3004: unknown type '" + n + "'");
    }

    Type expr(const ExprPtr& e) {
        if (!e) throw CheckError("T1002: missing expression");

        switch (e->kind) {
        case Expr::Literal:
            if (std::holds_alternative<std::monostate>(e->literal.data)) return {Kind::Null, {}};
            if (std::holds_alternative<bool>(e->literal.data)) return {Kind::Bool, {}};
            if (std::holds_alternative<std::int64_t>(e->literal.data)) return {Kind::Int, {}};
            if (std::holds_alternative<double>(e->literal.data)) return {Kind::Float, {}};
            if (std::holds_alternative<std::string>(e->literal.data)) return {Kind::String, {}};
            return {Kind::Unknown, {}};

        case Expr::Variable:
            return find(e->name);

        case Expr::Array: {
            Type element{Kind::Unknown, {}};
            for (auto& a : e->args) {
                auto t = expr(a);
                if (element.kind == Kind::Unknown) element = t;
                else if (!same(element, t))
                    throw CheckError("T3022: array elements must have compatible types");
            }
            return {Kind::Array, {}};
        }

        case Expr::Unary: {
            const auto t = expr(e->right);
            if (e->op == "-" && !numeric(t))
                throw CheckError("T3002: unary '-' requires int or float");
            if (e->op == "!" && t.kind != Kind::Bool && t.kind != Kind::Unknown)
                throw CheckError("T3002: '!' requires bool");
            return e->op == "!" ? Type{Kind::Bool, {}} : t;
        }

        case Expr::Binary: {
            const auto a = expr(e->left);
            const auto b = expr(e->right);

            if (e->op == "+" && a.kind == Kind::String && b.kind == Kind::String)
                return {Kind::String, {}};

            if (e->op == "+" || e->op == "-" || e->op == "*" ||
                e->op == "/" || e->op == "%") {
                if (!numeric(a) || !numeric(b))
                    throw CheckError("T3002: operator '" + e->op + "' requires numeric operands");
                return merge_numeric(a, b);
            }

            if (e->op == "&&" || e->op == "||") {
                if ((a.kind != Kind::Bool && a.kind != Kind::Unknown) ||
                    (b.kind != Kind::Bool && b.kind != Kind::Unknown))
                    throw CheckError("T3002: logical operator requires bool operands");
                return {Kind::Bool, {}};
            }

            if (e->op == "==" || e->op == "!=" ||
                e->op == "<" || e->op == "<=" ||
                e->op == ">" || e->op == ">=")
                return {Kind::Bool, {}};

            throw CheckError("T3002: unknown operator '" + e->op + "'");
        }

        case Expr::Member: {
            const auto o = expr(e->object);
            if (o.kind == Kind::Unknown) return {Kind::Unknown, {}};
            if (o.kind != Kind::User)
                throw CheckError("T3023: member access requires a struct or class value");
            const auto it = structs.find(o.custom);
            if (it == structs.end())
                throw CheckError("T3024: type '" + o.custom + "' has no declared members");
            const auto field = it->second.fields.find(e->name);
            if (field == it->second.fields.end())
                throw CheckError("T3025: type '" + o.custom + "' has no member '" + e->name + "'");
            return field->second;
        }

        case Expr::Index: {
            const auto a = expr(e->left);
            const auto i = expr(e->index);
            if (a.kind != Kind::Array && a.kind != Kind::Unknown)
                throw CheckError("T3012: indexing requires an array");
            if (i.kind != Kind::Int && i.kind != Kind::Unknown)
                throw CheckError("T3013: array index requires int");
            return {Kind::Unknown, {}};
        }

        case Expr::Call: {
            if (!e->left || e->left->kind != Expr::Variable)
                throw CheckError("T3014: call target must be a function");

            const auto name = e->left->name;
            const auto st = structs.find(name);
            if (st != structs.end()) {
                if (st->second.fields.size() != e->args.size())
                    throw CheckError("T3015: wrong argument count for '" + name + "'");
                std::size_t i = 0;
                for (const auto& [field, expected] : st->second.fields) {
                    const auto actual = expr(e->args[i++]);
                    if (!same(expected, actual))
                        throw CheckError("T3026: constructor argument type mismatch for '" + name + "'");
                }
                return {Kind::User, name};
            }

            if (name == "web_write") {
                for (auto& a : e->args) expr(a);
                return {Kind::Void, {}};
            }

            const auto it = functions.find(name);
            if (it == functions.end())
                throw CheckError("T2001: undefined function '" + name + "'");

            if (it->second.params.size() != e->args.size())
                throw CheckError("T3015: wrong argument count for '" + name + "'");

            for (std::size_t i = 0; i < e->args.size(); ++i) {
                const auto actual = expr(e->args[i]);
                if (!same(it->second.params[i], actual))
                    throw CheckError("T3027: argument " + std::to_string(i + 1) +
                                     " type mismatch for '" + name + "'");
            }
            return it->second.result;
        }
        }

        return {Kind::Unknown, {}};
    }

    void body(const std::vector<StmtPtr>& b, bool in_function = false, const Type* expected_return = nullptr) {
        for (const auto& s : b) {
            if (!s) throw CheckError("T1003: null statement");

            switch (s->kind) {
            case Stmt::Struct: {
                if (structs.count(s->name))
                    throw CheckError("T2003: duplicate type '" + s->name + "'");
                StructSig sig;
                if (s->fields.size() != s->field_types.size())
                    throw CheckError("T3028: struct field metadata is inconsistent");
                for (std::size_t i = 0; i < s->fields.size(); ++i) {
                    if (sig.fields.count(s->fields[i]))
                        throw CheckError("T3029: duplicate field '" + s->fields[i] + "'");
                    sig.fields.emplace(s->fields[i], parse_type(s->field_types[i]));
                }
                structs.emplace(s->name, std::move(sig));
                break;
            }

            case Stmt::Enum:
                if (enums.count(s->name))
                    throw CheckError("T2003: duplicate type '" + s->name + "'");
                enums[s->name] = s->enum_values;
                if (scopes.back().count(s->name))
                    throw CheckError("T2002: duplicate binding '" + s->name + "'");
                scopes.back()[s->name] = {Kind::User, s->name};
                break;

            case Stmt::Match: {
                const auto target = expr(s->match_expr);
                for (auto& branch : s->match_cases) {
                    const auto pattern = expr(branch.first);
                    if (!same(target, pattern))
                        throw CheckError("T3030: match pattern type does not match target");
                    scopes.push_back({});
                    body(branch.second, in_function, expected_return);
                    scopes.pop_back();
                }
                scopes.push_back({});
                body(s->match_default, in_function, expected_return);
                scopes.pop_back();
                break;
            }

            case Stmt::Let: {
                auto t = expr(s->expr);
                if (!s->type_name.empty()) {
                    const auto declared = parse_type(s->type_name);
                    if (!same(t, declared))
                        throw CheckError("T3001: initializer type does not match '" + s->type_name + "'");
                    t = declared;
                }
                if (scopes.back().count(s->name))
                    throw CheckError("T2002: duplicate binding '" + s->name + "'");
                scopes.back()[s->name] = t;
                break;
            }

            case Stmt::Assign: {
                const auto now = expr(s->expr);
                if (s->target && s->target->kind == Expr::Member) {
                    const auto old = expr(s->target);
                    if (!same(old, now))
                        throw CheckError("T3001: member assignment type mismatch");
                } else {
                    const auto old = find(s->name);
                    if (!same(old, now))
                        throw CheckError("T3001: cannot assign " + now.name() +
                                         " to " + old.name() + " variable '" + s->name + "'");
                }
                break;
            }

            case Stmt::Print:
                for (auto& a : s->print_args) expr(a);
                break;

            case Stmt::ExprStmt:
                expr(s->expr);
                break;

            case Stmt::If:
                for (auto& x : s->branches) {
                    const auto t = expr(x.first);
                    if (t.kind != Kind::Bool && t.kind != Kind::Unknown)
                        throw CheckError("T3003: if condition requires bool");
                    scopes.push_back({});
                    body(x.second, in_function, expected_return);
                    scopes.pop_back();
                }
                scopes.push_back({});
                body(s->else_body, in_function, expected_return);
                scopes.pop_back();
                break;

            case Stmt::For: {
                const auto a = expr(s->for_start);
                const auto b2 = expr(s->for_end);
                if ((a.kind != Kind::Int && a.kind != Kind::Unknown) ||
                    (b2.kind != Kind::Int && b2.kind != Kind::Unknown))
                    throw CheckError("T3005: for range requires int bounds");
                scopes.push_back({});
                scopes.back()[s->name] = {Kind::Int, {}};
                body(s->body, in_function, expected_return);
                scopes.pop_back();
                break;
            }

            case Stmt::While: {
                const auto t = expr(s->expr);
                if (t.kind != Kind::Bool && t.kind != Kind::Unknown)
                    throw CheckError("T3003: while condition requires bool");
                scopes.push_back({});
                body(s->body, in_function, expected_return);
                scopes.pop_back();
                break;
            }

            case Stmt::Function:
                break;

            case Stmt::Return:
                if (!in_function)
                    throw CheckError("T3016: return outside function");
                if (!expected_return)
                    break;
                if (expected_return->kind == Kind::Void) {
                    if (s->expr)
                        throw CheckError("T3031: void function cannot return a value");
                } else {
                    if (!s->expr)
                        throw CheckError("T3032: non-void function must return a value");
                    const auto actual = expr(s->expr);
                    if (!same(*expected_return, actual))
                        throw CheckError("T3033: return type mismatch: expected " +
                                         expected_return->name() + ", got " + actual.name());
                }
                break;

            case Stmt::Break:
            case Stmt::Continue:
                break;

            case Stmt::Throw:
                expr(s->expr);
                break;

            case Stmt::Try:
                scopes.push_back({});
                body(s->body, in_function, expected_return);
                scopes.pop_back();
                scopes.push_back({});
                if (!s->catch_name.empty())
                    scopes.back()[s->catch_name] = {Kind::String, {}};
                body(s->catch_body, in_function, expected_return);
                scopes.pop_back();
                scopes.push_back({});
                body(s->finally_body, in_function, expected_return);
                scopes.pop_back();
                break;

            case Stmt::WebFile:
                for (auto& a : s->web_parts) expr(a);
                break;

            default:
                break;
            }
        }
    }

    void run(const Program& p) {
        scopes.push_back({});

        // Register all declarations first so forward references are valid.
        for (const auto& s : p.statements) {
            if (s->kind == Stmt::Struct) {
                if (structs.count(s->name))
                    throw CheckError("T2003: duplicate type '" + s->name + "'");
                StructSig sig;
                for (std::size_t i = 0; i < s->fields.size(); ++i) {
                    if (i >= s->field_types.size())
                        throw CheckError("T3028: struct field type is missing");
                    sig.fields.emplace(s->fields[i], parse_type(s->field_types[i]));
                }
                structs.emplace(s->name, std::move(sig));
            }
            if (s->kind == Stmt::Enum) {
                if (enums.count(s->name))
                    throw CheckError("T2003: duplicate type '" + s->name + "'");
                enums[s->name] = s->enum_values;
                scopes.back()[s->name] = {Kind::User, s->name};
            }
        }

        // Function signatures are also collected before bodies are checked.
        for (const auto& s : p.statements) {
            if (s->kind != Stmt::Function) continue;
            if (functions.count(s->name))
                throw CheckError("T2003: duplicate function '" + s->name + "'");
            FunctionSig sig;
            for (std::size_t i = 0; i < s->params.size(); ++i) {
                if (i < s->param_types.size() && !s->param_types[i].empty())
                    sig.params.push_back(parse_type(s->param_types[i]));
                else
                    sig.params.push_back({Kind::Unknown, {}});
            }
            sig.result = s->return_type.empty() ? Type{Kind::Unknown, {}} : parse_type(s->return_type);
            functions.emplace(s->name, std::move(sig));
        }

        body(p.statements, false);
        for (const auto& s : p.statements) {
            if (s->kind != Stmt::Function) continue;
            const auto it = functions.find(s->name);
            scopes.push_back({});
            for (std::size_t i = 0; i < s->params.size(); ++i)
                scopes.back()[s->params[i]] = it->second.params[i];
            body(s->function_body, true, &it->second.result);
            scopes.pop_back();
        }
    }
};

} // namespace

void check(const Program& p) {
    Checker{}.run(p);
}

} // namespace ternet::types
