#include "types.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ternet::types {

std::string Type::name() const {
    switch (kind) {
    case Kind::Null: return "null";
    case Kind::Bool: return "bool";
    case Kind::Int: return "int";
    case Kind::Float: return "float";
    case Kind::Char: return "char";
    case Kind::Byte: return "byte";
    case Kind::String: return "String";
    case Kind::Array: return "Array<" + (args.empty() ? "unknown" : args[0].name()) + ">";
    case Kind::List: return "List<" + (args.empty() ? "unknown" : args[0].name()) + ">";
    case Kind::Map: return "Map<" + (args.size() > 0 ? args[0].name() : "unknown") + ", " +
                           (args.size() > 1 ? args[1].name() : "unknown") + ">";
    case Kind::Tuple: {
        std::string s = "(";
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i > 0) s += ", ";
            s += args[i].name();
        }
        s += ")";
        return s;
    }
    case Kind::Option: return "Option<" + (args.empty() ? "unknown" : args[0].name()) + ">";
    case Kind::Result: return "Result<" + (args.size() > 0 ? args[0].name() : "unknown") + ", " +
                              (args.size() > 1 ? args[1].name() : "unknown") + ">";
    case Kind::Void: return "void";
    case Kind::Never: return "never";
    case Kind::User:
    case Kind::Class:
    case Kind::Trait:
    case Kind::GenericParam:
        return custom.empty() ? "user" : custom;
    default: return "unknown";
    }
}

namespace {

struct FunctionSig {
    std::vector<Type> params;
    Type result;
    bool is_virtual = false;
    bool is_override = false;
    bool is_static = false;
};

struct StructSig {
    std::unordered_map<std::string, Type> fields;
    std::vector<std::string> field_order;
};

struct ClassSig {
    std::string name;
    std::string base_class;
    std::vector<std::string> traits;
    std::unordered_map<std::string, Type> fields;
    std::unordered_map<std::string, FunctionSig> methods;
    std::unordered_map<std::string, FunctionSig> static_methods;
    std::vector<std::string> generic_params;
};

struct TraitSig {
    std::string name;
    std::unordered_map<std::string, FunctionSig> methods;
};

struct Checker {
    std::vector<std::unordered_map<std::string, Type>> scopes;
    std::unordered_map<std::string, FunctionSig> functions;
    std::unordered_map<std::string, StructSig> structs;
    std::unordered_map<std::string, std::vector<std::string>> enums;
    std::unordered_map<std::string, ClassSig> classes;
    std::unordered_map<std::string, TraitSig> traits;
    std::string current_class;

    Type find(const std::string& n, SourcePos pos = {1, 1}) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto p = it->find(n);
            if (p != it->end()) return p->second;
        }
        // If in a class, check class fields and methods
        if (!current_class.empty()) {
            auto cls_it = classes.find(current_class);
            if (cls_it != classes.end()) {
                auto f_it = cls_it->second.fields.find(n);
                if (f_it != cls_it->second.fields.end()) return f_it->second;
            }
        }
        throw CheckError("T2001: undefined name '" + n + "'", pos, "T2001");
    }

    bool is_subclass_of(const std::string& derived, const std::string& base) const {
        if (derived == base) return true;
        std::string cur = derived;
        std::unordered_set<std::string> visited;
        while (!cur.empty()) {
            if (visited.count(cur)) break;
            visited.insert(cur);
            auto it = classes.find(cur);
            if (it == classes.end()) break;
            // Check traits
            for (const auto& tr : it->second.traits) {
                if (tr == base) return true;
            }
            if (it->second.base_class == base) return true;
            cur = it->second.base_class;
        }
        return false;
    }

    bool assignable_to(const Type& from, const Type& to) const {
        if (to.kind == Kind::Unknown || from.kind == Kind::Unknown) return true;
        if (to.kind == Kind::Never || from.kind == Kind::Never) return true;

        // Null can be assigned to Option, Object, Class, or Null
        if (from.kind == Kind::Null) {
            return to.kind == Kind::Null || to.kind == Kind::Option ||
                   to.kind == Kind::Class || to.kind == Kind::User;
        }

        // Numeric assignability: int and float
        if (to.kind == Kind::Float && from.kind == Kind::Int) return true;
        if (to.kind == Kind::Int && from.kind == Kind::Int) return true;
        if (to.kind == Kind::Float && from.kind == Kind::Float) return true;

        // Class inheritance and trait polymorphism
        if ((to.kind == Kind::Class || to.kind == Kind::User || to.kind == Kind::Trait) &&
            (from.kind == Kind::Class || from.kind == Kind::User)) {
            return is_subclass_of(from.custom, to.custom);
        }

        // Lists / Arrays
        if ((to.kind == Kind::List || to.kind == Kind::Array) &&
            (from.kind == Kind::List || from.kind == Kind::Array)) {
            if (to.args.empty() || from.args.empty()) return true;
            return assignable_to(from.args[0], to.args[0]);
        }

        // Tuples
        if (to.kind == Kind::Tuple && from.kind == Kind::Tuple) {
            if (to.args.size() != from.args.size()) return false;
            for (std::size_t i = 0; i < to.args.size(); ++i) {
                if (!assignable_to(from.args[i], to.args[i])) return false;
            }
            return true;
        }

        // Option
        if (to.kind == Kind::Option && from.kind == Kind::Option) {
            if (to.args.empty() || from.args.empty()) return true;
            return assignable_to(from.args[0], to.args[0]);
        }

        // Result
        if (to.kind == Kind::Result && from.kind == Kind::Result) {
            bool ok_match = (to.args.size() <= 0 || from.args.size() <= 0) || assignable_to(from.args[0], to.args[0]);
            bool err_match = (to.args.size() <= 1 || from.args.size() <= 1) || assignable_to(from.args[1], to.args[1]);
            return ok_match && err_match;
        }

        if (to.kind == from.kind) {
            if (to.kind == Kind::User || to.kind == Kind::Class || to.kind == Kind::Trait) {
                return to.custom == from.custom;
            }
            return true;
        }

        return false;
    }

    bool same(const Type& a, const Type& b) const {
        return assignable_to(a, b) && assignable_to(b, a);
    }

    static bool numeric(const Type& t) {
        return t.kind == Kind::Int || t.kind == Kind::Float || t.kind == Kind::Unknown;
    }

    static Type merge_numeric(const Type& a, const Type& b) {
        if (a.kind == Kind::Float || b.kind == Kind::Float) return Type::float_type();
        if (a.kind == Kind::Int && b.kind == Kind::Int) return Type::int_type();
        return Type::unknown();
    }

    Type parse_type_str(const std::string& n, SourcePos pos = {1, 1}) const {
        if (n.empty()) return Type::unknown();
        if (n == "int" || n == "size" || n == "int8" || n == "int16" ||
            n == "int32" || n == "int64" || n == "uint" || n == "uint8" ||
            n == "uint16" || n == "uint32" || n == "uint64" || n == "byte")
            return Type::int_type();
        if (n == "str" || n == "string" || n == "String") return Type::string_type();
        if (n == "float" || n == "float32" || n == "float64") return Type::float_type();
        if (n == "bool") return Type::bool_type();
        if (n == "char") return {Kind::Char, {}, {}};
        if (n == "void") return Type::void_type();
        if (n == "null") return Type::null_type();
        if (n == "never") return Type::never_type();

        // Generic: List<T>, Array<T>, Map<K,V>, Option<T>, Result<T,E>
        if (n.size() > 5 && n.substr(0, 5) == "List<" && n.back() == '>') {
            std::string elem_t = n.substr(5, n.size() - 6);
            return Type::list(parse_type_str(elem_t, pos));
        }
        if (n.size() > 6 && n.substr(0, 6) == "Array<" && n.back() == '>') {
            std::string elem_t = n.substr(6, n.size() - 7);
            return Type::array(parse_type_str(elem_t, pos));
        }
        if (n.size() > 7 && n.substr(0, 7) == "Option<" && n.back() == '>') {
            std::string elem_t = n.substr(7, n.size() - 8);
            return Type::option(parse_type_str(elem_t, pos));
        }
        if (n.size() > 7 && n.substr(0, 7) == "Result<" && n.back() == '>') {
            std::string inner = n.substr(7, n.size() - 8);
            auto comma = inner.find(',');
            if (comma != std::string::npos) {
                return Type::result(parse_type_str(inner.substr(0, comma), pos),
                                   parse_type_str(inner.substr(comma + 1), pos));
            }
            return Type::result(parse_type_str(inner, pos), Type::string_type());
        }

        // Tuple type: (T1, T2, ...)
        if (n.front() == '(' && n.back() == ')') {
            std::vector<Type> elem_types;
            std::string inner = n.substr(1, n.size() - 2);
            std::stringstream ss(inner);
            std::string item;
            while (std::getline(ss, item, ',')) {
                // trim item
                while (!item.empty() && item.front() == ' ') item.erase(item.begin());
                while (!item.empty() && item.back() == ' ') item.pop_back();
                if (!item.empty()) elem_types.push_back(parse_type_str(item, pos));
            }
            return Type::tuple(elem_types);
        }

        if (classes.count(n)) return Type::class_type(n);
        if (traits.count(n)) return Type::trait_type(n);
        if (structs.count(n)) return Type::user(n);
        if (enums.count(n)) return Type::user(n);

        // Treat unknown identifiers as User / Generic type
        return Type::user(n);
    }

    Type expr(const ExprPtr& e) {
        if (!e) throw CheckError("T1002: missing expression");

        switch (e->kind) {
        case Expr::Literal:
            if (std::holds_alternative<std::monostate>(e->literal.data)) return Type::null_type();
            if (std::holds_alternative<bool>(e->literal.data)) return Type::bool_type();
            if (std::holds_alternative<std::int64_t>(e->literal.data)) return Type::int_type();
            if (std::holds_alternative<double>(e->literal.data)) return Type::float_type();
            if (std::holds_alternative<std::string>(e->literal.data)) return Type::string_type();
            if (auto arr = std::get_if<Value::Array>(&e->literal.data)) {
                Type elem_t = Type::unknown();
                if (!arr->empty()) elem_t = Type::int_type();
                return Type::list(elem_t);
            }
            return Type::unknown();

        case Expr::Variable:
            return find(e->name, e->pos);

        case Expr::This: {
            if (current_class.empty())
                throw CheckError("T3050: 'this' cannot be used outside class methods", e->pos, "T3050");
            return Type::class_type(current_class);
        }

        case Expr::Super: {
            if (current_class.empty())
                throw CheckError("T3051: 'super' cannot be used outside class methods", e->pos, "T3051");
            const auto& cls = classes.at(current_class);
            if (cls.base_class.empty())
                throw CheckError("T3052: class '" + current_class + "' has no base class for 'super'", e->pos, "T3052");
            return Type::class_type(cls.base_class);
        }

        case Expr::Array: {
            Type element = Type::unknown();
            for (auto& a : e->args) {
                auto t = expr(a);
                if (element.kind == Kind::Unknown) element = t;
                else if (!assignable_to(t, element) && !assignable_to(element, t)) {
                    throw CheckError("T3022: array elements must have compatible types", e->pos, "T3022");
                }
            }
            return Type::list(element);
        }

        case Expr::Tuple: {
            std::vector<Type> element_types;
            for (auto& a : e->args) {
                element_types.push_back(expr(a));
            }
            return Type::tuple(element_types);
        }

        case Expr::Unary: {
            const auto t = expr(e->right);
            if (e->op == "-" && !numeric(t))
                throw CheckError("T3002: unary '-' requires int or float", e->pos, "T3002");
            if (e->op == "!" && t.kind != Kind::Bool && t.kind != Kind::Unknown)
                throw CheckError("T3002: '!' requires bool", e->pos, "T3002");
            if (e->op == "~" && t.kind != Kind::Int && t.kind != Kind::Unknown)
                throw CheckError("T3002: '~' requires int", e->pos, "T3002");
            return e->op == "!" ? Type::bool_type() : t;
        }

        case Expr::Binary: {
            const auto a = expr(e->left);
            const auto b = expr(e->right);

            if (e->op == "+" && (a.kind == Kind::String || b.kind == Kind::String)) {
                return Type::string_type();
            }

            if (e->op == "+" || e->op == "-" || e->op == "*" || e->op == "/" || e->op == "%") {
                if (!numeric(a) || !numeric(b))
                    throw CheckError("T3002: operator '" + e->op + "' requires numeric operands", e->pos, "T3002");
                return merge_numeric(a, b);
            }

            if (e->op == "&&" || e->op == "||") {
                if ((a.kind != Kind::Bool && a.kind != Kind::Unknown) ||
                    (b.kind != Kind::Bool && b.kind != Kind::Unknown))
                    throw CheckError("T3002: logical operator requires bool operands", e->pos, "T3002");
                return Type::bool_type();
            }

            if (e->op == "&" || e->op == "|" || e->op == "^" || e->op == "<<" || e->op == ">>") {
                if ((a.kind != Kind::Int && a.kind != Kind::Unknown) ||
                    (b.kind != Kind::Int && b.kind != Kind::Unknown))
                    throw CheckError("T3002: bitwise operator requires int operands", e->pos, "T3002");
                return Type::int_type();
            }

            if (e->op == "==" || e->op == "!=" ||
                e->op == "<" || e->op == "<=" ||
                e->op == ">" || e->op == ">=") {
                return Type::bool_type();
            }

            throw CheckError("T3002: unknown operator '" + e->op + "'", e->pos, "T3002");
        }

        case Expr::Member: {
            const auto o = expr(e->object);

            // Tuple indexing: user.0, user.1
            if (o.kind == Kind::Tuple) {
                try {
                    std::size_t idx = static_cast<std::size_t>(std::stoul(e->name));
                    if (idx >= o.args.size())
                        throw CheckError("T3017: tuple index " + e->name + " out of bounds", e->pos, "T3017");
                    return o.args[idx];
                } catch (const std::invalid_argument&) {
                    throw CheckError("T3025: invalid tuple member '" + e->name + "'", e->pos, "T3025");
                }
            }

            // List methods: append, pop, len, length, insert, remove, clear
            if (o.kind == Kind::List || o.kind == Kind::Array) {
                if (e->name == "len" || e->name == "length") return Type::int_type();
                if (e->name == "append") return Type::void_type();
                if (e->name == "pop") return o.args.empty() ? Type::unknown() : o.args[0];
                if (e->name == "clear") return Type::void_type();
                if (e->name == "insert") return Type::void_type();
                if (e->name == "remove") return o.args.empty() ? Type::unknown() : o.args[0];
                return Type::unknown();
            }

            // String methods: length, len, substr, contains, starts_with, ends_with, to_upper, to_lower, trim
            if (o.kind == Kind::String) {
                if (e->name == "len" || e->name == "length") return Type::int_type();
                if (e->name == "substr" || e->name == "to_upper" || e->name == "to_lower" || e->name == "trim")
                    return Type::string_type();
                if (e->name == "contains" || e->name == "starts_with" || e->name == "ends_with")
                    return Type::bool_type();
                return Type::unknown();
            }

            // Class fields & methods
            if (o.kind == Kind::Class || o.kind == Kind::User) {
                auto it = classes.find(o.custom);
                if (it != classes.end()) {
                    // Check fields
                    auto f_it = it->second.fields.find(e->name);
                    if (f_it != it->second.fields.end()) return f_it->second;

                    // Check methods
                    auto m_it = it->second.methods.find(e->name);
                    if (m_it != it->second.methods.end()) return m_it->second.result;

                    // Check base class
                    std::string cur = it->second.base_class;
                    while (!cur.empty()) {
                        auto base_it = classes.find(cur);
                        if (base_it == classes.end()) break;
                        auto bf = base_it->second.fields.find(e->name);
                        if (bf != base_it->second.fields.end()) return bf->second;
                        auto bm = base_it->second.methods.find(e->name);
                        if (bm != base_it->second.methods.end()) return bm->second.result;
                        cur = base_it->second.base_class;
                    }
                }

                auto s_it = structs.find(o.custom);
                if (s_it != structs.end()) {
                    auto f = s_it->second.fields.find(e->name);
                    if (f != s_it->second.fields.end()) return f->second;
                }

                // Check Enum
                auto enum_it = enums.find(o.custom);
                if (enum_it != enums.end()) {
                    for (const auto& v : enum_it->second) {
                        if (v == e->name) return Type::user(o.custom);
                    }
                }
            }

            // Trait methods
            if (o.kind == Kind::Trait) {
                auto tr_it = traits.find(o.custom);
                if (tr_it != traits.end()) {
                    auto m_it = tr_it->second.methods.find(e->name);
                    if (m_it != tr_it->second.methods.end()) return m_it->second.result;
                }
            }

            return Type::unknown();
        }

        case Expr::Index: {
            const auto a = expr(e->left);
            const auto i = expr(e->index);
            if (a.kind != Kind::Array && a.kind != Kind::List && a.kind != Kind::String && a.kind != Kind::Tuple && a.kind != Kind::Unknown)
                throw CheckError("T3012: indexing requires an array, list, or tuple", e->pos, "T3012");
            if (i.kind != Kind::Int && i.kind != Kind::Unknown)
                throw CheckError("T3013: array index requires int", e->pos, "T3013");
            if ((a.kind == Kind::List || a.kind == Kind::Array) && !a.args.empty()) {
                return a.args[0];
            }
            if (a.kind == Kind::String) return Type::string_type();
            return Type::unknown();
        }

        case Expr::Call: {
            // Check if call is on a member expression: obj.method(args)
            if (e->left && e->left->kind == Expr::Member) {
                auto obj_t = expr(e->left->object);
                std::string method_name = e->left->name;

                // Built-in list methods
                if (obj_t.kind == Kind::List || obj_t.kind == Kind::Array) {
                    if (method_name == "append") {
                        if (e->args.size() != 1)
                            throw CheckError("T3015: append() takes exactly 1 argument", e->pos, "T3015");
                        auto item_t = expr(e->args[0]);
                        if (!obj_t.args.empty() && !assignable_to(item_t, obj_t.args[0]))
                            throw CheckError("T3001: append item type mismatch", e->pos, "T3001");
                        return Type::void_type();
                    }
                    if (method_name == "pop") return obj_t.args.empty() ? Type::unknown() : obj_t.args[0];
                    if (method_name == "len" || method_name == "length") return Type::int_type();
                    if (method_name == "insert") return Type::void_type();
                    if (method_name == "remove") return obj_t.args.empty() ? Type::unknown() : obj_t.args[0];
                    if (method_name == "clear") return Type::void_type();
                }

                // Built-in string methods
                if (obj_t.kind == Kind::String) {
                    if (method_name == "len" || method_name == "length") return Type::int_type();
                    if (method_name == "substr" || method_name == "to_upper" || method_name == "to_lower" || method_name == "trim")
                        return Type::string_type();
                    if (method_name == "contains" || method_name == "starts_with" || method_name == "ends_with")
                        return Type::bool_type();
                }

                // Class method dispatch
                if (obj_t.kind == Kind::Class || obj_t.kind == Kind::User) {
                    auto it = classes.find(obj_t.custom);
                    if (it != classes.end()) {
                        FunctionSig sig;
                        bool found = false;
                        auto m_it = it->second.methods.find(method_name);
                        if (m_it != it->second.methods.end()) {
                            sig = m_it->second;
                            found = true;
                        } else {
                            std::string cur = it->second.base_class;
                            while (!cur.empty()) {
                                auto b_it = classes.find(cur);
                                if (b_it == classes.end()) break;
                                auto bm = b_it->second.methods.find(method_name);
                                if (bm != b_it->second.methods.end()) {
                                    sig = bm->second;
                                    found = true;
                                    break;
                                }
                                cur = b_it->second.base_class;
                            }
                        }
                        if (found) {
                            if (sig.params.size() != e->args.size())
                                throw CheckError("T3015: wrong argument count for method '" + method_name + "'", e->pos, "T3015");
                            for (std::size_t idx = 0; idx < e->args.size(); ++idx) {
                                auto arg_t = expr(e->args[idx]);
                                if (!assignable_to(arg_t, sig.params[idx]))
                                    throw CheckError("T3027: argument " + std::to_string(idx + 1) +
                                                     " type mismatch for method '" + method_name + "'", e->pos, "T3027");
                            }
                            return sig.result;
                        }
                    }
                }

                // Trait method dispatch
                if (obj_t.kind == Kind::Trait) {
                    auto tr_it = traits.find(obj_t.custom);
                    if (tr_it != traits.end()) {
                        auto m_it = tr_it->second.methods.find(method_name);
                        if (m_it != tr_it->second.methods.end()) {
                            for (auto& a : e->args) expr(a);
                            return m_it->second.result;
                        }
                    }
                }

                for (auto& a : e->args) expr(a);
                return Type::unknown();
            }

            if (!e->left || e->left->kind != Expr::Variable) {
                // Call on expression
                for (auto& a : e->args) expr(a);
                return Type::unknown();
            }

            const auto name = e->left->name;

            // Option and Result built-ins
            if (name == "some") {
                if (e->args.size() != 1) throw CheckError("T3034: some() expects one value", e->pos, "T3034");
                return Type::option(expr(e->args[0]));
            }
            if (name == "none") {
                if (!e->args.empty()) throw CheckError("T3034: none() expects no arguments", e->pos, "T3034");
                return Type::option(Type::unknown());
            }
            if (name == "ok") {
                if (e->args.size() != 1) throw CheckError("T3035: ok() expects one value", e->pos, "T3035");
                return Type::result(expr(e->args[0]), Type::unknown());
            }
            if (name == "err") {
                if (e->args.size() != 1) throw CheckError("T3035: err() expects one value", e->pos, "T3035");
                return Type::result(Type::unknown(), expr(e->args[0]));
            }
            if (name == "is_some" || name == "is_none" || name == "is_ok" || name == "is_err") {
                if (e->args.size() != 1) throw CheckError("T3036: " + name + "() expects one value", e->pos, "T3036");
                expr(e->args[0]);
                return Type::bool_type();
            }
            if (name == "unwrap" || name == "unwrap_or") {
                if (e->args.empty()) throw CheckError("T3037: unwrap expects arguments", e->pos, "T3037");
                const auto opt = expr(e->args[0]);
                if (e->args.size() > 1) {
                    return expr(e->args[1]);
                }
                return opt.args.empty() ? Type::unknown() : opt.args[0];
            }

            // Built-in functions
            if (name == "print" || name == "tnprint") {
                for (auto& a : e->args) expr(a);
                return Type::void_type();
            }
            if (name == "len") {
                if (e->args.size() != 1) throw CheckError("T3015: len() expects 1 argument", e->pos, "T3015");
                expr(e->args[0]);
                return Type::int_type();
            }
            if (name == "input" || name == "input_string") {
                return Type::string_type();
            }
            if (name == "input_int") return Type::int_type();
            if (name == "input_float") return Type::float_type();
            if (name == "input_bool") return Type::bool_type();

            // Class instantiation: Dog("Buddy", 3)
            auto cls_it = classes.find(name);
            if (cls_it != classes.end()) {
                auto init_it = cls_it->second.methods.find("init");
                if (init_it != cls_it->second.methods.end()) {
                    if (init_it->second.params.size() != e->args.size())
                        throw CheckError("T3015: constructor for '" + name + "' expects " +
                                         std::to_string(init_it->second.params.size()) + " arguments", e->pos, "T3015");
                    for (std::size_t idx = 0; idx < e->args.size(); ++idx) {
                        auto arg_t = expr(e->args[idx]);
                        if (!assignable_to(arg_t, init_it->second.params[idx]))
                            throw CheckError("T3026: constructor argument type mismatch for '" + name + "'", e->pos, "T3026");
                    }
                } else {
                    for (auto& a : e->args) expr(a);
                }
                return Type::class_type(name);
            }

            // Struct instantiation: User("Ajmal", 15)
            auto st = structs.find(name);
            if (st != structs.end()) {
                if (st->second.fields.size() != e->args.size())
                    throw CheckError("T3015: wrong argument count for struct '" + name + "'", e->pos, "T3015");
                std::size_t idx = 0;
                for (const auto& field_name : st->second.field_order) {
                    auto expected = st->second.fields.at(field_name);
                    const auto actual = expr(e->args[idx++]);
                    if (!assignable_to(actual, expected))
                        throw CheckError("T3026: constructor argument type mismatch for '" + name + "'", e->pos, "T3026");
                }
                return Type::user(name);
            }

            if (name == "web_write") {
                for (auto& a : e->args) expr(a);
                return Type::void_type();
            }

            // Standard functions
            const auto it = functions.find(name);
            if (it == functions.end()) {
                // If it's in scope as a variable or generic function
                try {
                    auto var_t = find(name, e->pos);
                    if (var_t.kind == Kind::Function) return var_t.args.back();
                } catch (...) {}
                // Allow generic call or forward reference
                for (auto& a : e->args) expr(a);
                return Type::unknown();
            }

            if (it->second.params.size() != e->args.size())
                throw CheckError("T3015: wrong argument count for '" + name + "'", e->pos, "T3015");

            for (std::size_t idx = 0; idx < e->args.size(); ++idx) {
                const auto actual = expr(e->args[idx]);
                if (!assignable_to(actual, it->second.params[idx]))
                    throw CheckError("T3027: argument " + std::to_string(idx + 1) +
                                     " type mismatch for '" + name + "'", e->pos, "T3027");
            }
            return it->second.result;
        }
        }

        return Type::unknown();
    }

    void body(const std::vector<StmtPtr>& b, bool in_function = false, const Type* expected_return = nullptr) {
        for (const auto& s : b) {
            if (!s) continue;

            switch (s->kind) {
            case Stmt::Struct: {
                if (structs.count(s->name))
                    throw CheckError("T2003: duplicate type '" + s->name + "'", s->pos, "T2003");
                StructSig sig;
                for (std::size_t i = 0; i < s->fields.size(); ++i) {
                    if (sig.fields.count(s->fields[i]))
                        throw CheckError("T3029: duplicate field '" + s->fields[i] + "'", s->pos, "T3029");
                    auto ft = parse_type_str(s->field_types[i], s->pos);
                    sig.fields.emplace(s->fields[i], ft);
                    sig.field_order.push_back(s->fields[i]);
                }
                structs.emplace(s->name, std::move(sig));
                break;
            }

            case Stmt::Enum:
                if (enums.count(s->name))
                    throw CheckError("T2003: duplicate type '" + s->name + "'", s->pos, "T2003");
                enums[s->name] = s->enum_values;
                if (!scopes.empty() && scopes.back().count(s->name))
                    throw CheckError("T2002: duplicate binding '" + s->name + "'", s->pos, "T2002");
                if (!scopes.empty()) scopes.back()[s->name] = Type::user(s->name);
                break;

            case Stmt::Class: {
                ClassSig sig;
                sig.name = s->name;
                sig.base_class = s->base_name;
                sig.traits = s->traits;
                sig.generic_params = s->generic_params;

                for (const auto& m : s->methods) {
                    if (m->kind == Stmt::Let) {
                        sig.fields[m->name] = parse_type_str(m->type_name, m->pos);
                    } else if (m->kind == Stmt::Function) {
                        FunctionSig fsig;
                        for (std::size_t i = 0; i < m->params.size(); ++i) {
                            std::string pt = (i < m->param_types.size()) ? m->param_types[i] : "";
                            fsig.params.push_back(parse_type_str(pt, m->pos));
                        }
                        fsig.result = parse_type_str(m->return_type, m->pos);
                        fsig.is_virtual = m->is_virtual;
                        fsig.is_override = m->is_override;
                        fsig.is_static = m->is_static;
                        if (m->is_static) sig.static_methods[m->name] = fsig;
                        else sig.methods[m->name] = fsig;
                    }
                }
                classes[s->name] = std::move(sig);
                if (!scopes.empty()) scopes.back()[s->name] = Type::class_type(s->name);
                break;
            }

            case Stmt::Trait: {
                TraitSig sig;
                sig.name = s->name;
                for (const auto& m : s->methods) {
                    if (m->kind == Stmt::Function) {
                        FunctionSig fsig;
                        for (std::size_t i = 0; i < m->params.size(); ++i) {
                            std::string pt = (i < m->param_types.size()) ? m->param_types[i] : "";
                            fsig.params.push_back(parse_type_str(pt, m->pos));
                        }
                        fsig.result = parse_type_str(m->return_type, m->pos);
                        sig.methods[m->name] = fsig;
                    }
                }
                traits[s->name] = std::move(sig);
                break;
            }

            case Stmt::Match: {
                const auto target = expr(s->match_expr);
                for (auto& mc : s->match_cases) {
                    scopes.push_back({});
                    if (!mc.pattern_var.empty()) {
                        // Bind payload variable
                        Type payload = target.args.empty() ? Type::unknown() : target.args[0];
                        scopes.back()[mc.pattern_var] = payload;
                    } else if (mc.pattern) {
                        const auto pat_t = expr(mc.pattern);
                        if (!assignable_to(pat_t, target) && !assignable_to(target, pat_t))
                            throw CheckError("T3030: match pattern type does not match target", s->pos, "T3030");
                    }
                    body(mc.body, in_function, expected_return);
                    scopes.pop_back();
                }
                break;
            }

            case Stmt::Let: {
                auto val_t = s->expr ? expr(s->expr) : Type::unknown();
                if (!s->type_name.empty()) {
                    const auto declared = parse_type_str(s->type_name, s->pos);
                    if (s->expr && !assignable_to(val_t, declared)) {
                        throw CheckError("T3001: initializer type (" + val_t.name() +
                                         ") does not match declared type '" + s->type_name + "'",
                                         s->pos, "T3001");
                    }
                    val_t = declared;
                }
                if (!scopes.empty() && scopes.back().count(s->name))
                    throw CheckError("T2002: duplicate binding '" + s->name + "'", s->pos, "T2002");
                if (!scopes.empty()) scopes.back()[s->name] = val_t;
                break;
            }

            case Stmt::Assign: {
                const auto now = expr(s->expr);
                if (s->target && s->target->kind == Expr::Member) {
                    const auto old = expr(s->target);
                    if (!assignable_to(now, old))
                        throw CheckError("T3001: member assignment type mismatch", s->pos, "T3001");
                } else if (s->target && s->target->kind == Expr::Index) {
                    const auto elem = expr(s->target);
                    if (!assignable_to(now, elem))
                        throw CheckError("T3001: element assignment type mismatch", s->pos, "T3001");
                } else {
                    const auto old = find(s->name, s->pos);
                    if (!assignable_to(now, old))
                        throw CheckError("T3001: cannot assign " + now.name() +
                                         " to " + old.name() + " variable '" + s->name + "'",
                                         s->pos, "T3001");
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
                        throw CheckError("T3003: if condition requires bool", s->pos, "T3003");
                    scopes.push_back({});
                    body(x.second, in_function, expected_return);
                    scopes.pop_back();
                }
                scopes.push_back({});
                body(s->else_body, in_function, expected_return);
                scopes.pop_back();
                break;

            case Stmt::For: {
                if (s->for_end) {
                    const auto a = expr(s->for_start);
                    const auto b2 = expr(s->for_end);
                    if ((a.kind != Kind::Int && a.kind != Kind::Unknown) ||
                        (b2.kind != Kind::Int && b2.kind != Kind::Unknown))
                        throw CheckError("T3005: for range requires int bounds", s->pos, "T3005");
                    scopes.push_back({});
                    scopes.back()[s->name] = Type::int_type();
                    body(s->body, in_function, expected_return);
                    scopes.pop_back();
                } else {
                    // Collection for-in
                    const auto coll = expr(s->for_start);
                    Type item_t = coll.args.empty() ? Type::unknown() : coll.args[0];
                    scopes.push_back({});
                    scopes.back()[s->name] = item_t;
                    body(s->body, in_function, expected_return);
                    scopes.pop_back();
                }
                break;
            }

            case Stmt::While: {
                const auto t = expr(s->expr);
                if (t.kind != Kind::Bool && t.kind != Kind::Unknown)
                    throw CheckError("T3003: while condition requires bool", s->pos, "T3003");
                scopes.push_back({});
                body(s->body, in_function, expected_return);
                scopes.pop_back();
                break;
            }

            case Stmt::Return:
                if (!in_function)
                    throw CheckError("T3016: return outside function", s->pos, "T3016");
                if (!expected_return) break;
                if (expected_return->kind == Kind::Void) {
                    if (s->expr)
                        throw CheckError("T3031: void function cannot return a value", s->pos, "T3031");
                } else {
                    if (!s->expr)
                        throw CheckError("T3032: non-void function must return a value", s->pos, "T3032");
                    const auto actual = expr(s->expr);
                    if (!assignable_to(actual, *expected_return))
                        throw CheckError("T3033: return type mismatch: expected " +
                                         expected_return->name() + ", got " + actual.name(),
                                         s->pos, "T3033");
                }
                break;

            case Stmt::Try:
                scopes.push_back({});
                body(s->body, in_function, expected_return);
                scopes.pop_back();
                scopes.push_back({});
                if (!s->catch_name.empty())
                    scopes.back()[s->catch_name] = Type::string_type();
                body(s->catch_body, in_function, expected_return);
                scopes.pop_back();
                scopes.push_back({});
                body(s->finally_body, in_function, expected_return);
                scopes.pop_back();
                break;

            case Stmt::Throw:
                expr(s->expr);
                break;

            case Stmt::WebFile:
                for (auto& a : s->web_parts) expr(a);
                break;

            default:
                break;
            }
        }
    }

    void validate_class_contracts() {
        for (const auto& [name, cls] : classes) {
            // Check inheritance cycle
            std::string cur = cls.base_class;
            std::unordered_set<std::string> visited = {name};
            while (!cur.empty()) {
                if (visited.count(cur))
                    throw CheckError("T3041: circular inheritance detected for class '" + name + "'");
                visited.insert(cur);
                auto it = classes.find(cur);
                if (it == classes.end()) break;
                cur = it->second.base_class;
            }

            // Check trait implementation contracts
            for (const auto& trait_name : cls.traits) {
                auto tr_it = traits.find(trait_name);
                if (tr_it == traits.end()) continue;
                for (const auto& [req_m, req_sig] : tr_it->second.methods) {
                    auto m_it = cls.methods.find(req_m);
                    if (m_it == cls.methods.end()) {
                        // Check base classes
                        bool found_in_base = false;
                        std::string b = cls.base_class;
                        while (!b.empty()) {
                            auto base_cls = classes.find(b);
                            if (base_cls != classes.end() && base_cls->second.methods.count(req_m)) {
                                found_in_base = true;
                                break;
                            }
                            if (base_cls != classes.end()) b = base_cls->second.base_class;
                            else break;
                        }
                        if (!found_in_base) {
                            throw CheckError("T3040: class '" + name + "' does not implement required method '" +
                                             req_m + "' from trait '" + trait_name + "'");
                        }
                    }
                }
            }
        }
    }

    void check_all_method_bodies() {
        for (const auto& [cls_name, cls] : classes) {
            current_class = cls_name;
            for (const auto& [method_name, fsig] : cls.methods) {
                scopes.push_back({});
                // 'this' is in scope
                scopes.back()["this"] = Type::class_type(cls_name);
                for (const auto& [fname, ftype] : cls.fields) {
                    scopes.back()[fname] = ftype;
                }
                for (std::size_t i = 0; i < fsig.params.size(); ++i) {
                    scopes.back()["p" + std::to_string(i)] = fsig.params[i];
                }
                // Find method AST
                for (const auto& stmt_item : cls.fields) {}
                scopes.pop_back();
            }
            current_class = "";
        }
    }

    void run(const Program& p) {
        scopes.push_back({});

        // 1. Pass 1: Declare all structs, enums, traits, classes
        for (const auto& s : p.statements) {
            if (!s) continue;
            if (s->kind == Stmt::Struct) {
                if (structs.count(s->name))
                    throw CheckError("T2003: duplicate type '" + s->name + "'", s->pos, "T2003");
                StructSig sig;
                for (std::size_t i = 0; i < s->fields.size(); ++i) {
                    auto ft = parse_type_str(i < s->field_types.size() ? s->field_types[i] : "", s->pos);
                    sig.fields.emplace(s->fields[i], ft);
                    sig.field_order.push_back(s->fields[i]);
                }
                structs.emplace(s->name, std::move(sig));
            }
            if (s->kind == Stmt::Enum) {
                enums[s->name] = s->enum_values;
                scopes.back()[s->name] = Type::user(s->name);
            }
            if (s->kind == Stmt::Trait) {
                TraitSig sig;
                sig.name = s->name;
                for (const auto& m : s->methods) {
                    if (m->kind == Stmt::Function) {
                        FunctionSig fsig;
                        for (std::size_t i = 0; i < m->params.size(); ++i) {
                            std::string pt = (i < m->param_types.size()) ? m->param_types[i] : "";
                            fsig.params.push_back(parse_type_str(pt, m->pos));
                        }
                        fsig.result = parse_type_str(m->return_type, m->pos);
                        sig.methods[m->name] = fsig;
                    }
                }
                traits[s->name] = std::move(sig);
            }
            if (s->kind == Stmt::Class) {
                ClassSig sig;
                sig.name = s->name;
                sig.base_class = s->base_name;
                sig.traits = s->traits;
                sig.generic_params = s->generic_params;
                for (const auto& m : s->methods) {
                    if (m->kind == Stmt::Let) {
                        sig.fields[m->name] = parse_type_str(m->type_name, m->pos);
                    } else if (m->kind == Stmt::Function) {
                        FunctionSig fsig;
                        for (std::size_t i = 0; i < m->params.size(); ++i) {
                            std::string pt = (i < m->param_types.size()) ? m->param_types[i] : "";
                            fsig.params.push_back(parse_type_str(pt, m->pos));
                        }
                        fsig.result = parse_type_str(m->return_type, m->pos);
                        fsig.is_virtual = m->is_virtual;
                        fsig.is_override = m->is_override;
                        fsig.is_static = m->is_static;
                        if (m->is_static) sig.static_methods[m->name] = fsig;
                        else sig.methods[m->name] = fsig;
                    }
                }
                classes[s->name] = std::move(sig);
                scopes.back()[s->name] = Type::class_type(s->name);
            }
        }

        // Validate contracts across all classes and traits
        validate_class_contracts();

        // 2. Pass 2: Declare top-level functions
        for (const auto& s : p.statements) {
            if (!s || s->kind != Stmt::Function) continue;
            FunctionSig sig;
            for (std::size_t i = 0; i < s->params.size(); ++i) {
                if (i < s->param_types.size() && !s->param_types[i].empty())
                    sig.params.push_back(parse_type_str(s->param_types[i], s->pos));
                else
                    sig.params.push_back(Type::unknown());
            }
            sig.result = s->return_type.empty() ? Type::unknown() : parse_type_str(s->return_type, s->pos);
            functions.emplace(s->name, std::move(sig));
        }

        // 3. Pass 3: Check top-level statements
        body(p.statements, false);

        // 4. Pass 4: Check function bodies
        for (const auto& s : p.statements) {
            if (!s || s->kind != Stmt::Function) continue;
            const auto it = functions.find(s->name);
            scopes.push_back({});
            for (std::size_t i = 0; i < s->params.size(); ++i)
                scopes.back()[s->params[i]] = it->second.params[i];
            body(s->function_body, true, &it->second.result);
            scopes.pop_back();
        }

        // 5. Pass 5: Check class method bodies
        for (const auto& s : p.statements) {
            if (!s || s->kind != Stmt::Class) continue;
            current_class = s->name;
            const auto& cls_sig = classes.at(s->name);
            for (const auto& m : s->methods) {
                if (!m || m->kind != Stmt::Function) continue;
                scopes.push_back({});
                // 'this' binding
                scopes.back()["this"] = Type::class_type(s->name);
                // Bind all fields in scope for method body
                for (const auto& [fn, ft] : cls_sig.fields) {
                    scopes.back()[fn] = ft;
                }
                for (std::size_t i = 0; i < m->params.size(); ++i) {
                    std::string pt = (i < m->param_types.size()) ? m->param_types[i] : "";
                    scopes.back()[m->params[i]] = parse_type_str(pt, m->pos);
                }
                Type ret_t = parse_type_str(m->return_type, m->pos);
                body(m->function_body, true, &ret_t);
                scopes.pop_back();
            }
            current_class = "";
        }
    }
};

} // namespace

void check(const Program& p) {
    Checker{}.run(p);
}

} // namespace ternet::types
