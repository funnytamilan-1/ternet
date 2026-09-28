#include "types.hpp"
#include <unordered_map>

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
    default: return "unknown";
    }
}

namespace {

struct Checker {
    std::vector<std::unordered_map<std::string, Type>> scopes;
    std::unordered_map<std::string, std::vector<Type>> functions;

    Type find(const std::string& n) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto p = it->find(n);
            if (p != it->end()) return p->second;
        }
        throw CheckError("T2001: undefined name '" + n + "'");
    }

    static bool numeric(Type t) {
        return t.kind == Kind::Int || t.kind == Kind::Float || t.kind == Kind::Unknown;
    }

    static Type merge_numeric(Type a, Type b) {
        if (a.kind == Kind::Float || b.kind == Kind::Float)
            return {Kind::Float};
        if (a.kind == Kind::Int && b.kind == Kind::Int)
            return {Kind::Int};
        return {Kind::Unknown};
    }

    Type expr(const ExprPtr& e) {
        if (!e) throw CheckError("T1002: missing expression");

        switch (e->kind) {
        case Expr::Literal:
            if (std::holds_alternative<std::monostate>(e->literal.data)) return {Kind::Null};
            if (std::holds_alternative<bool>(e->literal.data)) return {Kind::Bool};
            if (std::holds_alternative<std::int64_t>(e->literal.data)) return {Kind::Int};
            if (std::holds_alternative<double>(e->literal.data)) return {Kind::Float};
            if (std::holds_alternative<std::string>(e->literal.data)) return {Kind::String};
            return {Kind::Unknown};

        case Expr::Variable:
            return find(e->name);

        case Expr::Array:
            for (auto& a : e->args) expr(a);
            return {Kind::Array};

        case Expr::Unary: {
            const auto t = expr(e->right);
            if (e->op == "-" && !numeric(t))
                throw CheckError("T3002: unary '-' requires int or float");
            if (e->op == "!" && t.kind != Kind::Bool && t.kind != Kind::Unknown)
                throw CheckError("T3002: '!' requires bool");
            return e->op == "!" ? Type{Kind::Bool} : t;
        }

        case Expr::Binary: {
            const auto a = expr(e->left);
            const auto b = expr(e->right);

            if (e->op == "+" && a.kind == Kind::String && b.kind == Kind::String)
                return {Kind::String};

            if (e->op == "+" || e->op == "-" || e->op == "*" ||
                e->op == "/" || e->op == "%") {
                if (!numeric(a) || !numeric(b))
                    throw CheckError("T3002: operator '" + e->op + "' requires numeric operands");
                return merge_numeric(a, b);
            }

            if (e->op == "&&" || e->op == "||") {
                if ((a.kind != Kind::Bool && a.kind != Kind::Unknown) ||
                    (b.kind != Kind::Bool && b.kind != Kind::Unknown)) {
                    throw CheckError("T3002: logical operator requires bool operands");
                }
                return {Kind::Bool};
            }

            if (e->op == "==" || e->op == "!=" ||
                e->op == "<" || e->op == "<=" ||
                e->op == ">" || e->op == ">=") {
                return {Kind::Bool};
            }

            throw CheckError("T3002: unknown operator '" + e->op + "'");
        }

        case Expr::Index: {
            const auto a = expr(e->left);
            const auto i = expr(e->index);
            if (a.kind != Kind::Array && a.kind != Kind::Unknown)
                throw CheckError("T3012: indexing requires an array");
            if (i.kind != Kind::Int && i.kind != Kind::Unknown)
                throw CheckError("T3013: array index requires int");
            return {Kind::Unknown};
        }

        case Expr::Call: {
            if (!e->left || e->left->kind != Expr::Variable)
                throw CheckError("T3014: call target must be a function");

            if (e->left->name == "web_write") {
                for (auto& a : e->args) expr(a);
                return {Kind::Void};
            }

            const auto it = functions.find(e->left->name);
            if (it == functions.end())
                throw CheckError("T2001: undefined function '" + e->left->name + "'");

            if (it->second.size() != e->args.size())
                throw CheckError("T3015: wrong argument count for '" + e->left->name + "'");

            for (auto& a : e->args) expr(a);
            return {Kind::Unknown};
        }
        }

        return {Kind::Unknown};
    }

    void body(const std::vector<StmtPtr>& b, bool in_function = false) {
        for (const auto& s : b) {
            if (!s) throw CheckError("T1003: null statement");

            switch (s->kind) {
            case Stmt::Let: {
                auto t = expr(s->expr);
                if (!s->type_name.empty()) {
                    if (s->type_name=="int"||s->type_name=="size") t.kind=Kind::Int;
                    else if (s->type_name=="str") t.kind=Kind::String;
                    else if (s->type_name=="float") t.kind=Kind::Float;
                    else if (s->type_name=="bool") t.kind=Kind::Bool;
                    else if (s->type_name=="void") t.kind=Kind::Void;
                    else throw CheckError("T3004: unknown type '" + s->type_name + "'");
                }
                if (scopes.back().count(s->name))
                    throw CheckError("T2002: duplicate binding '" + s->name + "'");
                scopes.back()[s->name] = t;
                break;
            }

            case Stmt::Assign: {
                const auto old = find(s->name);
                const auto now = expr(s->expr);
                if (old.kind != Kind::Unknown && now.kind != Kind::Unknown &&
                    old.kind != now.kind) {
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
                    body(x.second, in_function);
                    scopes.pop_back();
                }
                scopes.push_back({});
                body(s->else_body, in_function);
                scopes.pop_back();
                break;

            case Stmt::For: {
                const auto a=expr(s->for_start), b=expr(s->for_end);
                if ((a.kind!=Kind::Int&&a.kind!=Kind::Unknown)||(b.kind!=Kind::Int&&b.kind!=Kind::Unknown))
                    throw CheckError("T3005: for range requires int bounds");
                scopes.push_back({}); scopes.back()[s->name]={Kind::Int}; body(s->body,in_function); scopes.pop_back();
                break;
            }

            case Stmt::While: {
                const auto t = expr(s->expr);
                if (t.kind != Kind::Bool && t.kind != Kind::Unknown)
                    throw CheckError("T3003: while condition requires bool");
                scopes.push_back({});
                body(s->body, in_function);
                scopes.pop_back();
                break;
            }

            case Stmt::Function: {
                scopes.push_back({});
                for (std::size_t i=0;i<s->params.size();++i) {
                    Type t{Kind::Unknown};
                    if (i<s->param_types.size()&&!s->param_types[i].empty()) {
                        if(s->param_types[i]=="int"||s->param_types[i]=="size") t={Kind::Int};
                        else if(s->param_types[i]=="str") t={Kind::String};
                        else if(s->param_types[i]=="float") t={Kind::Float};
                        else if(s->param_types[i]=="bool") t={Kind::Bool};
                        else throw CheckError("T3004: unknown parameter type '"+s->param_types[i]+"'");
                    }
                    scopes.back()[s->params[i]]=t;
                }
                body(s->function_body, true);
                scopes.pop_back();
                break;
            }

            case Stmt::Return:
                if (!in_function)
                    throw CheckError("T3016: return outside function");
                if (s->expr) expr(s->expr);
                break;

            case Stmt::Break:
            case Stmt::Continue:
                break;

            case Stmt::Throw:
                expr(s->expr);
                break;

            case Stmt::Try:
                scopes.push_back({});
                body(s->body, in_function);
                scopes.pop_back();

                scopes.push_back({});
                if (!s->catch_name.empty())
                    scopes.back()[s->catch_name] = {Kind::String};
                body(s->catch_body, in_function);
                scopes.pop_back();

                scopes.push_back({});
                body(s->finally_body, in_function);
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

        for (const auto& s : p.statements) {
            if (s->kind == Stmt::Function) {
                if (functions.count(s->name))
                    throw CheckError("T2003: duplicate function '" + s->name + "'");
                functions[s->name] =
                    std::vector<Type>(s->params.size(), Type{Kind::Unknown});
            }
        }

        body(p.statements, false);
    }
};

} // namespace

void check(const Program& p) {
    Checker{}.run(p);
}

} // namespace ternet::types
