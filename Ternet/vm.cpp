#include "ternet.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>

namespace fs = std::filesystem;

namespace ternet {

bool Value::truthy() const {
    if (std::holds_alternative<std::monostate>(data)) return false;
    if (auto p = std::get_if<bool>(&data)) return *p;
    if (auto p = std::get_if<std::int64_t>(&data)) return *p != 0;
    if (auto p = std::get_if<double>(&data)) return *p != 0.0 && !std::isnan(*p);
    if (auto p = std::get_if<std::string>(&data)) return !p->empty();
    if (auto p = std::get_if<Array>(&data)) return !p->empty();
    if (auto p = std::get_if<Tuple>(&data)) return !p->items.empty();
    if (auto p = std::get_if<Object>(&data)) return !p->empty();
    return true;
}

std::string Value::type_name() const {
    if (std::holds_alternative<std::monostate>(data)) return "null";
    if (std::holds_alternative<bool>(data)) return "bool";
    if (std::holds_alternative<std::int64_t>(data)) return "int";
    if (std::holds_alternative<double>(data)) return "float";
    if (std::holds_alternative<std::string>(data)) return "String";
    if (std::holds_alternative<Array>(data)) return "List";
    if (std::holds_alternative<Tuple>(data)) return "Tuple";
    if (auto o = std::get_if<Object>(&data)) {
        auto it = o->find("__class__");
        if (it != o->end()) return it->second.str();
        auto tag_it = o->find("tag");
        if (tag_it != o->end()) return tag_it->second.str();
        return "Object";
    }
    return "unknown";
}

std::string Value::str() const {
    if (std::holds_alternative<std::monostate>(data)) return "null";
    if (auto p = std::get_if<bool>(&data)) return *p ? "true" : "false";
    if (auto p = std::get_if<std::int64_t>(&data)) return std::to_string(*p);
    if (auto p = std::get_if<double>(&data)) {
        auto s = std::to_string(*p);
        while (s.size() > 1 && s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
        return s;
    }
    if (auto p = std::get_if<std::string>(&data)) return *p;
    if (auto p = std::get_if<Array>(&data)) {
        std::string s = "[";
        for (std::size_t i = 0; i < p->size(); ++i) {
            if (i > 0) s += ", ";
            s += (*p)[i].str();
        }
        return s + "]";
    }
    if (auto p = std::get_if<Tuple>(&data)) {
        std::string s = "(";
        for (std::size_t i = 0; i < p->items.size(); ++i) {
            if (i > 0) s += ", ";
            s += p->items[i].str();
        }
        return s + ")";
    }
    if (auto p = std::get_if<Object>(&data)) {
        // Tagged Option / Result
        auto tag_it = p->find("tag");
        if (tag_it != p->end()) {
            std::string tag = tag_it->second.str();
            if (tag == "none") return "none";
            auto val_it = p->find("value");
            if (val_it != p->end()) {
                return tag + "(" + val_it->second.str() + ")";
            }
            return tag;
        }

        // Class instance
        auto cls_it = p->find("__class__");
        if (cls_it != p->end()) {
            std::string s = cls_it->second.str() + " {";
            bool first = true;
            for (const auto& [k, v] : *p) {
                if (k == "__class__") continue;
                if (!first) s += ", ";
                first = false;
                s += k + ": " + v.str();
            }
            return s + "}";
        }

        // Generic object / map
        std::string s = "{";
        bool first = true;
        for (const auto& [k, v] : *p) {
            if (!first) s += ", ";
            first = false;
            s += k + ": " + v.str();
        }
        return s + "}";
    }
    return "<object>";
}

Interpreter::Binding* Interpreter::find(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto p = it->find(name);
        if (p != it->end()) return &p->second;
    }
    return nullptr;
}

Value Interpreter::binary(const std::string& op, const Value& a, const Value& b) {
    if (op == "&&") return a.truthy() && b.truthy();
    if (op == "||") return a.truthy() || b.truthy();

    if (op == "==") {
        if (a.data.index() == b.data.index()) {
            if (std::holds_alternative<std::monostate>(a.data)) return true;
            if (auto p1 = std::get_if<bool>(&a.data)) return *p1 == std::get<bool>(b.data);
            if (auto p1 = std::get_if<std::int64_t>(&a.data)) return *p1 == std::get<std::int64_t>(b.data);
            if (auto p1 = std::get_if<double>(&a.data)) return *p1 == std::get<double>(b.data);
            if (auto p1 = std::get_if<std::string>(&a.data)) return *p1 == std::get<std::string>(b.data);
        }
        if ((std::holds_alternative<std::int64_t>(a.data) || std::holds_alternative<double>(a.data)) &&
            (std::holds_alternative<std::int64_t>(b.data) || std::holds_alternative<double>(b.data))) {
            double v1 = std::holds_alternative<std::int64_t>(a.data) ? std::get<std::int64_t>(a.data) : std::get<double>(a.data);
            double v2 = std::holds_alternative<std::int64_t>(b.data) ? std::get<std::int64_t>(b.data) : std::get<double>(b.data);
            return v1 == v2;
        }
        return a.str() == b.str();
    }
    if (op == "!=") {
        return !binary("==", a, b).truthy();
    }

    auto num = [](const Value& v) -> double {
        if (auto p = std::get_if<std::int64_t>(&v.data)) return static_cast<double>(*p);
        if (auto p = std::get_if<double>(&v.data)) return *p;
        throw RuntimeError("numeric operator requires numbers");
    };

    if (op == "+") {
        if (std::holds_alternative<std::string>(a.data) || std::holds_alternative<std::string>(b.data)) {
            return a.str() + b.str();
        }
        if (auto arr_a = std::get_if<Value::Array>(&a.data)) {
            if (auto arr_b = std::get_if<Value::Array>(&b.data)) {
                Value::Array combined = *arr_a;
                combined.insert(combined.end(), arr_b->begin(), arr_b->end());
                return combined;
            }
        }
        if (std::holds_alternative<std::int64_t>(a.data) && std::holds_alternative<std::int64_t>(b.data)) {
            return std::get<std::int64_t>(a.data) + std::get<std::int64_t>(b.data);
        }
        return num(a) + num(b);
    }

    if (std::holds_alternative<std::int64_t>(a.data) && std::holds_alternative<std::int64_t>(b.data)) {
        std::int64_t x = std::get<std::int64_t>(a.data);
        std::int64_t y = std::get<std::int64_t>(b.data);
        if (op == "-") return x - y;
        if (op == "*") return x * y;
        if (op == "/") {
            if (y == 0) throw RuntimeError("division by zero");
            return x / y;
        }
        if (op == "%") {
            if (y == 0) throw RuntimeError("modulo by zero");
            return x % y;
        }
        if (op == "&") return x & y;
        if (op == "|") return x | y;
        if (op == "^") return x ^ y;
        if (op == "<<") return x << y;
        if (op == ">>") return x >> y;
        if (op == ">") return x > y;
        if (op == "<") return x < y;
        if (op == ">=") return x >= y;
        if (op == "<=") return x <= y;
    }

    double x = num(a);
    double y = num(b);
    if (op == "-") return x - y;
    if (op == "*") return x * y;
    if (op == "/") {
        if (y == 0.0) throw RuntimeError("division by zero");
        return x / y;
    }
    if (op == "%") {
        if (y == 0.0) throw RuntimeError("modulo by zero");
        return std::fmod(x, y);
    }
    if (op == ">") return x > y;
    if (op == "<") return x < y;
    if (op == ">=") return x >= y;
    if (op == "<=") return x <= y;

    throw RuntimeError("unknown operator " + op);
}

Value Interpreter::instantiate_class(const ClassDef& cls, const std::vector<Value>& args) {
    Value::Object obj;
    obj["__class__"] = Value(cls.name);

    // Initialize all fields from base classes down to derived
    std::vector<std::string> class_chain;
    std::string cur = cls.name;
    while (!cur.empty()) {
        class_chain.push_back(cur);
        auto it = classes.find(cur);
        if (it != classes.end()) cur = it->second.base_name;
        else break;
    }
    std::reverse(class_chain.begin(), class_chain.end());

    for (const auto& cname : class_chain) {
        auto it = classes.find(cname);
        if (it != classes.end()) {
            for (const auto& f : it->second.fields) {
                obj[f] = Value();
            }
        }
    }

    // Check if class defines constructor `init`
    bool init_called = false;
    for (auto it = class_chain.rbegin(); it != class_chain.rend(); ++it) {
        auto c_it = classes.find(*it);
        if (c_it != classes.end() && c_it->second.methods.count("init")) {
            const auto& init_fn = c_it->second.methods.at("init");
            if (init_fn.params.size() != args.size())
                throw RuntimeError("wrong argument count for constructor of '" + cls.name + "'");

            scopes.push_back({});
            scopes.back()["this"] = {Value(obj), true};
            for (std::size_t i = 0; i < args.size(); ++i) {
                scopes.back()[init_fn.params[i]] = {args[i], true};
            }
            // Bind fields in scope
            for (const auto& [k, v] : obj) {
                if (k != "__class__") scopes.back()[k] = {v, true};
            }

            returning = false;
            return_value = {};
            exec_all(init_fn.body);
            returning = false;

            // Sync modified field values from scope and 'this' back into obj
            for (const auto& [k, v] : scopes.back()) {
                if (k != "this" && k != "__class__" && obj.count(k)) {
                    if (!std::holds_alternative<std::monostate>(v.value.data)) {
                        obj[k] = v.value;
                    }
                }
            }
            if (auto this_b = scopes.back().find("this"); this_b != scopes.back().end()) {
                if (auto o_ptr = std::get_if<Value::Object>(&this_b->second.value.data)) {
                    for (const auto& [k, v] : *o_ptr) {
                        obj[k] = v;
                    }
                }
            }
            scopes.pop_back();
            init_called = true;
            break;
        }
    }

    // If no init method, initialize positional fields from args
    if (!init_called && !args.empty()) {
        std::size_t idx = 0;
        for (const auto& cname : class_chain) {
            auto it = classes.find(cname);
            if (it != classes.end()) {
                for (const auto& f : it->second.fields) {
                    if (idx < args.size()) {
                        obj[f] = args[idx++];
                    }
                }
            }
        }
    }

    return Value(std::move(obj));
}

Value Interpreter::call_method(const ClassDef& cls, Value::Object& obj, const std::string& method_name, const std::vector<Value>& args) {
    // Dynamic virtual dispatch: look for the method starting from dynamic class
    std::string cur_class = cls.name;
    const Function* fn_ptr = nullptr;

    while (!cur_class.empty()) {
        auto it = classes.find(cur_class);
        if (it == classes.end()) break;
        auto m_it = it->second.methods.find(method_name);
        if (m_it != it->second.methods.end()) {
            fn_ptr = &m_it->second;
            break;
        }
        cur_class = it->second.base_name;
    }

    if (!fn_ptr) {
        throw RuntimeError("undefined method '" + method_name + "' for class '" + cls.name + "'");
    }

    if (fn_ptr->params.size() != args.size()) {
        throw RuntimeError("wrong argument count for method '" + method_name + "'");
    }

    scopes.push_back({});
    scopes.back()["this"] = {Value(obj), true};
    for (std::size_t i = 0; i < args.size(); ++i) {
        scopes.back()[fn_ptr->params[i]] = {args[i], true};
    }
    for (const auto& [k, v] : obj) {
        if (k != "__class__") scopes.back()[k] = {v, true};
    }

    returning = false;
    return_value = {};
    exec_all(fn_ptr->body);
    auto r = return_value;
    returning = false;

    // Sync instance modifications back
    for (const auto& [k, v] : scopes.back()) {
        if (k != "this" && k != "__class__" && obj.count(k)) {
            if (!std::holds_alternative<std::monostate>(v.value.data)) {
                obj[k] = v.value;
            }
        }
    }
    if (auto this_b = scopes.back().find("this"); this_b != scopes.back().end()) {
        if (auto o_ptr = std::get_if<Value::Object>(&this_b->second.value.data)) {
            for (const auto& [k, v] : *o_ptr) obj[k] = v;
        }
    }

    scopes.pop_back();
    return r;
}

Value Interpreter::eval(const ExprPtr& e) {
    if (!e) throw RuntimeError("internal error: null expression");

    switch (e->kind) {
    case Expr::Literal:
        return e->literal;

    case Expr::Variable: {
        auto b = find(e->name);
        if (b) return b->value;

        // Check if it refers to an enum type
        auto enum_it = enums.find(e->name);
        if (enum_it != enums.end()) {
            Value::Object vals;
            for (const auto& v : enum_it->second.values) {
                vals[v] = Value(e->name + "." + v);
            }
            return Value(std::move(vals));
        }

        // Check if inside a class method and accessing field directly
        if (auto this_b = find("this")) {
            if (auto o = std::get_if<Value::Object>(&this_b->value.data)) {
                auto fit = o->find(e->name);
                if (fit != o->end()) return fit->second;
            }
        }

        throw RuntimeError("undefined variable '" + e->name + "'", e->pos, "T2001");
    }

    case Expr::This: {
        auto b = find("this");
        if (!b) throw RuntimeError("cannot use 'this' outside class methods", e->pos, "T3050");
        return b->value;
    }

    case Expr::Super: {
        auto b = find("this");
        if (!b) throw RuntimeError("cannot use 'super' outside class methods", e->pos, "T3051");
        return b->value;
    }

    case Expr::Unary: {
        auto v = eval(e->right);
        if (e->op == "!") return !v.truthy();
        if (e->op == "-") {
            if (auto p = std::get_if<std::int64_t>(&v.data)) return -(*p);
            if (auto p = std::get_if<double>(&v.data)) return -(*p);
        }
        if (e->op == "+") {
            if (std::holds_alternative<std::int64_t>(v.data) || std::holds_alternative<double>(v.data)) return v;
        }
        if (e->op == "~") {
            if (auto p = std::get_if<std::int64_t>(&v.data)) return ~(*p);
        }
        throw RuntimeError("invalid unary operator '" + e->op + "'", e->pos, "T3002");
    }

    case Expr::Binary:
        return binary(e->op, eval(e->left), eval(e->right));

    case Expr::Array: {
        Value::Array a;
        for (auto& q : e->args) a.push_back(eval(q));
        return Value(std::move(a));
    }

    case Expr::Tuple: {
        Value::Tuple v;
        for (auto& q : e->args) v.items.push_back(eval(q));
        return Value(std::move(v));
    }

    case Expr::Index: {
        auto v = eval(e->left);
        auto idx = eval(e->index);

        if (auto a = std::get_if<Value::Array>(&v.data)) {
            auto n = std::get_if<std::int64_t>(&idx.data);
            if (!n || *n < 0 || static_cast<std::size_t>(*n) >= a->size())
                throw RuntimeError("array index out of bounds", e->pos, "T3017");
            return (*a)[static_cast<std::size_t>(*n)];
        }
        if (auto s = std::get_if<std::string>(&v.data)) {
            auto n = std::get_if<std::int64_t>(&idx.data);
            if (!n || *n < 0 || static_cast<std::size_t>(*n) >= s->size())
                throw RuntimeError("string index out of bounds", e->pos, "T3017");
            return std::string(1, (*s)[static_cast<std::size_t>(*n)]);
        }
        if (auto t = std::get_if<Value::Tuple>(&v.data)) {
            auto n = std::get_if<std::int64_t>(&idx.data);
            if (!n || *n < 0 || static_cast<std::size_t>(*n) >= t->items.size())
                throw RuntimeError("tuple index out of bounds", e->pos, "T3017");
            return t->items[static_cast<std::size_t>(*n)];
        }
        throw RuntimeError("indexing requires an array, string, or tuple", e->pos, "T3012");
    }

    case Expr::Member: {
        auto v = eval(e->object);

        // Tuple member indexing: user.0, user.1
        if (auto t = std::get_if<Value::Tuple>(&v.data)) {
            try {
                std::size_t idx = static_cast<std::size_t>(std::stoul(e->name));
                if (idx >= t->items.size())
                    throw RuntimeError("tuple index " + e->name + " out of bounds", e->pos, "T3017");
                return t->items[idx];
            } catch (const std::invalid_argument&) {
                throw RuntimeError("invalid tuple index '" + e->name + "'", e->pos, "T3025");
            }
        }

        // List properties / methods
        if (auto a = std::get_if<Value::Array>(&v.data)) {
            if (e->name == "len" || e->name == "length") return static_cast<std::int64_t>(a->size());
        }

        // String properties / methods
        if (auto s = std::get_if<std::string>(&v.data)) {
            if (e->name == "len" || e->name == "length") return static_cast<std::int64_t>(s->size());
        }

        // Object / instance fields
        if (auto o = std::get_if<Value::Object>(&v.data)) {
            auto it = o->find(e->name);
            if (it != o->end()) return it->second;

            // Check if it is a class method
            auto cls_it = o->find("__class__");
            if (cls_it != o->end()) {
                auto c_it = classes.find(cls_it->second.str());
                if (c_it != classes.end() && c_it->second.methods.count(e->name)) {
                    return Value("<method " + e->name + ">");
                }
            }
            throw RuntimeError("unknown member '" + e->name + "'", e->pos, "T3025");
        }

        throw RuntimeError("member access requires an object or tuple", e->pos, "T3019");
    }

    case Expr::Call: {
        // Method Call: obj.method(args...)
        if (e->left && e->left->kind == Expr::Member) {
            auto obj_val = eval(e->left->object);
            std::string method_name = e->left->name;

            // Built-in List methods: append, pop, len, insert, remove, clear
            if (auto a_ptr = std::get_if<Value::Array>(&obj_val.data)) {
                if (method_name == "append") {
                    if (e->args.size() != 1) throw RuntimeError("append() expects 1 argument", e->pos, "T3015");
                    auto val = eval(e->args[0]);
                    a_ptr->push_back(val);
                    // Sync back to target variable if variable
                    if (e->left->object->kind == Expr::Variable) {
                        if (auto b = find(e->left->object->name)) b->value = obj_val;
                    }
                    return Value();
                }
                if (method_name == "pop") {
                    if (a_ptr->empty()) throw RuntimeError("pop() from empty list", e->pos, "T3017");
                    auto item = a_ptr->back();
                    a_ptr->pop_back();
                    if (e->left->object->kind == Expr::Variable) {
                        if (auto b = find(e->left->object->name)) b->value = obj_val;
                    }
                    return item;
                }
                if (method_name == "len" || method_name == "length") {
                    return static_cast<std::int64_t>(a_ptr->size());
                }
                if (method_name == "clear") {
                    a_ptr->clear();
                    if (e->left->object->kind == Expr::Variable) {
                        if (auto b = find(e->left->object->name)) b->value = obj_val;
                    }
                    return Value();
                }
                if (method_name == "insert") {
                    if (e->args.size() != 2) throw RuntimeError("insert() expects 2 arguments: (index, value)", e->pos, "T3015");
                    auto idx_val = eval(e->args[0]);
                    auto item_val = eval(e->args[1]);
                    auto idx = std::get_if<std::int64_t>(&idx_val.data);
                    if (!idx || *idx < 0 || static_cast<std::size_t>(*idx) > a_ptr->size())
                        throw RuntimeError("insert index out of bounds", e->pos, "T3017");
                    a_ptr->insert(a_ptr->begin() + static_cast<std::ptrdiff_t>(*idx), item_val);
                    if (e->left->object->kind == Expr::Variable) {
                        if (auto b = find(e->left->object->name)) b->value = obj_val;
                    }
                    return Value();
                }
                if (method_name == "remove") {
                    if (e->args.size() != 1) throw RuntimeError("remove() expects 1 argument: (index)", e->pos, "T3015");
                    auto idx_val = eval(e->args[0]);
                    auto idx = std::get_if<std::int64_t>(&idx_val.data);
                    if (!idx || *idx < 0 || static_cast<std::size_t>(*idx) >= a_ptr->size())
                        throw RuntimeError("remove index out of bounds", e->pos, "T3017");
                    auto item = (*a_ptr)[static_cast<std::size_t>(*idx)];
                    a_ptr->erase(a_ptr->begin() + static_cast<std::ptrdiff_t>(*idx));
                    if (e->left->object->kind == Expr::Variable) {
                        if (auto b = find(e->left->object->name)) b->value = obj_val;
                    }
                    return item;
                }
            }

            // Built-in String methods: substr, contains, starts_with, ends_with, to_upper, to_lower, trim, split
            if (auto s_ptr = std::get_if<std::string>(&obj_val.data)) {
                if (method_name == "len" || method_name == "length") {
                    return static_cast<std::int64_t>(s_ptr->size());
                }
                if (method_name == "substr") {
                    if (e->args.empty()) throw RuntimeError("substr expects at least 1 argument", e->pos, "T3015");
                    std::int64_t start = std::get<std::int64_t>(eval(e->args[0]).data);
                    std::int64_t len = (e->args.size() > 1) ? std::get<std::int64_t>(eval(e->args[1]).data) : static_cast<std::int64_t>(s_ptr->size() - start);
                    if (start < 0 || static_cast<std::size_t>(start) > s_ptr->size()) return std::string("");
                    return s_ptr->substr(static_cast<std::size_t>(start), static_cast<std::size_t>(std::max<std::int64_t>(0, len)));
                }
                if (method_name == "contains") {
                    if (e->args.size() != 1) throw RuntimeError("contains expects 1 argument", e->pos, "T3015");
                    std::string sub = eval(e->args[0]).str();
                    return s_ptr->find(sub) != std::string::npos;
                }
                if (method_name == "starts_with") {
                    if (e->args.size() != 1) throw RuntimeError("starts_with expects 1 argument", e->pos, "T3015");
                    std::string sub = eval(e->args[0]).str();
                    return s_ptr->rfind(sub, 0) == 0;
                }
                if (method_name == "ends_with") {
                    if (e->args.size() != 1) throw RuntimeError("ends_with expects 1 argument", e->pos, "T3015");
                    std::string sub = eval(e->args[0]).str();
                    if (sub.size() > s_ptr->size()) return false;
                    return s_ptr->compare(s_ptr->size() - sub.size(), sub.size(), sub) == 0;
                }
                if (method_name == "to_upper") {
                    std::string res = *s_ptr;
                    for (char& c : res) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    return res;
                }
                if (method_name == "to_lower") {
                    std::string res = *s_ptr;
                    for (char& c : res) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    return res;
                }
                if (method_name == "trim") {
                    std::string res = *s_ptr;
                    while (!res.empty() && std::isspace(static_cast<unsigned char>(res.front()))) res.erase(res.begin());
                    while (!res.empty() && std::isspace(static_cast<unsigned char>(res.back()))) res.pop_back();
                    return res;
                }
            }

            // Class instance method dispatch
            if (auto o_ptr = std::get_if<Value::Object>(&obj_val.data)) {
                auto cls_tag = o_ptr->find("__class__");
                if (cls_tag != o_ptr->end()) {
                    auto c_it = classes.find(cls_tag->second.str());
                    if (c_it != classes.end()) {
                        std::vector<Value> arg_vals;
                        for (auto& a : e->args) arg_vals.push_back(eval(a));
                        auto res = call_method(c_it->second, *o_ptr, method_name, arg_vals);
                        // Save modified instance back to variable
                        if (e->left->object->kind == Expr::Variable) {
                            if (auto b = find(e->left->object->name)) b->value = obj_val;
                        }
                        return res;
                    }
                }
            }

            throw RuntimeError("cannot call method '" + method_name + "' on object", e->pos, "T3025");
        }

        if (e->left->kind != Expr::Variable) {
            throw RuntimeError("call target must be a function or method", e->pos, "T3014");
        }

        const auto& name = e->left->name;

        // Standard I/O functions
        if (name == "print" || name == "tnprint") {
            for (std::size_t i = 0; i < e->args.size(); ++i) {
                if (i > 0) std::cout << ' ';
                std::cout << eval(e->args[i]).str();
            }
            std::cout << '\n';
            return Value();
        }

        if (name == "len") {
            if (e->args.size() != 1) throw RuntimeError("len() expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            if (auto a = std::get_if<Value::Array>(&v.data)) return static_cast<std::int64_t>(a->size());
            if (auto s = std::get_if<std::string>(&v.data)) return static_cast<std::int64_t>(s->size());
            if (auto t = std::get_if<Value::Tuple>(&v.data)) return static_cast<std::int64_t>(t->items.size());
            throw RuntimeError("len() requires a list, string, or tuple", e->pos, "T3012");
        }

        if (name == "input" || name == "input_string") {
            if (!e->args.empty()) throw RuntimeError(name + " expects no arguments", e->pos, "T3015");
            std::string value;
            if (!std::getline(std::cin, value)) throw RuntimeError("input reached EOF", e->pos, "E1002");
            return Value(value);
        }

        if (name == "input_int") {
            if (!e->args.empty()) throw RuntimeError("input_int expects no arguments", e->pos, "T3015");
            std::string value;
            if (!std::getline(std::cin, value)) throw RuntimeError("input reached EOF", e->pos, "E1002");
            try {
                return Value(static_cast<std::int64_t>(std::stoll(value)));
            } catch (...) {
                throw RuntimeError("input_int: expected integer input", e->pos, "E1003");
            }
        }

        if (name == "input_float") {
            if (!e->args.empty()) throw RuntimeError("input_float expects no arguments", e->pos, "T3015");
            std::string value;
            if (!std::getline(std::cin, value)) throw RuntimeError("input reached EOF", e->pos, "E1002");
            try {
                return Value(std::stod(value));
            } catch (...) {
                throw RuntimeError("input_float: expected float input", e->pos, "E1003");
            }
        }

        if (name == "input_bool") {
            if (!e->args.empty()) throw RuntimeError("input_bool expects no arguments", e->pos, "T3015");
            std::string value;
            if (!std::getline(std::cin, value)) throw RuntimeError("input reached EOF", e->pos, "E1002");
            for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (value == "true" || value == "1" || value == "yes" || value == "y") return Value(true);
            if (value == "false" || value == "0" || value == "no" || value == "n") return Value(false);
            throw RuntimeError("input_bool: expected boolean input", e->pos, "E1003");
        }

        // Option and Result constructors
        if (name == "some" || name == "ok" || name == "err") {
            if (e->args.size() != 1) throw RuntimeError(name + " expects 1 argument", e->pos, "T3015");
            Value::Object o;
            o["tag"] = Value(name);
            o["value"] = eval(e->args[0]);
            return Value(std::move(o));
        }

        if (name == "none") {
            if (!e->args.empty()) throw RuntimeError("none expects no arguments", e->pos, "T3015");
            Value::Object o;
            o["tag"] = Value("none");
            return Value(std::move(o));
        }

        if (name == "is_some" || name == "is_none") {
            if (e->args.size() != 1) throw RuntimeError(name + " expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            auto o = std::get_if<Value::Object>(&v.data);
            if (!o) return Value(false);
            auto it = o->find("tag");
            if (it == o->end()) return Value(false);
            return Value(it->second.str() == (name == "is_some" ? "some" : "none"));
        }

        if (name == "is_ok" || name == "is_err") {
            if (e->args.size() != 1) throw RuntimeError(name + " expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            auto o = std::get_if<Value::Object>(&v.data);
            if (!o) return Value(false);
            auto it = o->find("tag");
            if (it == o->end()) return Value(false);
            return Value(it->second.str() == (name == "is_ok" ? "ok" : "err"));
        }

        if (name == "unwrap") {
            if (e->args.size() != 1) throw RuntimeError("unwrap expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            auto o = std::get_if<Value::Object>(&v.data);
            if (!o) throw RuntimeError("unwrap called on non-Option/Result value", e->pos, "E1004");
            auto tag_it = o->find("tag");
            if (tag_it == o->end() || tag_it->second.str() == "none")
                throw RuntimeError("called unwrap() on none", e->pos, "E1004");
            if (tag_it->second.str() == "err")
                throw RuntimeError("called unwrap() on err: " + (*o)["value"].str(), e->pos, "E1004");
            return (*o)["value"];
        }

        if (name == "unwrap_or") {
            if (e->args.size() != 2) throw RuntimeError("unwrap_or expects 2 arguments", e->pos, "T3015");
            auto v = eval(e->args[0]);
            auto o = std::get_if<Value::Object>(&v.data);
            if (!o) return eval(e->args[1]);
            auto tag_it = o->find("tag");
            if (tag_it != o->end() && (tag_it->second.str() == "some" || tag_it->second.str() == "ok")) {
                return (*o)["value"];
            }
            return eval(e->args[1]);
        }

        // Standard Math Functions: abs, min, max, sqrt, pow, floor, ceil, round, random
        if (name == "abs") {
            if (e->args.size() != 1) throw RuntimeError("abs expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            if (auto p = std::get_if<std::int64_t>(&v.data)) return Value(std::abs(*p));
            if (auto p = std::get_if<double>(&v.data)) return Value(std::fabs(*p));
        }
        if (name == "min") {
            if (e->args.size() != 2) throw RuntimeError("min expects 2 arguments", e->pos, "T3015");
            return binary("<", eval(e->args[0]), eval(e->args[1])).truthy() ? eval(e->args[0]) : eval(e->args[1]);
        }
        if (name == "max") {
            if (e->args.size() != 2) throw RuntimeError("max expects 2 arguments", e->pos, "T3015");
            return binary(">", eval(e->args[0]), eval(e->args[1])).truthy() ? eval(e->args[0]) : eval(e->args[1]);
        }
        if (name == "sqrt") {
            if (e->args.size() != 1) throw RuntimeError("sqrt expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            double d = std::holds_alternative<std::int64_t>(v.data) ? std::get<std::int64_t>(v.data) : std::get<double>(v.data);
            return Value(std::sqrt(d));
        }
        if (name == "pow") {
            if (e->args.size() != 2) throw RuntimeError("pow expects 2 arguments", e->pos, "T3015");
            auto a = eval(e->args[0]);
            auto b = eval(e->args[1]);
            double d1 = std::holds_alternative<std::int64_t>(a.data) ? std::get<std::int64_t>(a.data) : std::get<double>(a.data);
            double d2 = std::holds_alternative<std::int64_t>(b.data) ? std::get<std::int64_t>(b.data) : std::get<double>(b.data);
            return Value(std::pow(d1, d2));
        }
        if (name == "floor") {
            if (e->args.size() != 1) throw RuntimeError("floor expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            double d = std::holds_alternative<std::int64_t>(v.data) ? std::get<std::int64_t>(v.data) : std::get<double>(v.data);
            return Value(static_cast<std::int64_t>(std::floor(d)));
        }
        if (name == "ceil") {
            if (e->args.size() != 1) throw RuntimeError("ceil expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            double d = std::holds_alternative<std::int64_t>(v.data) ? std::get<std::int64_t>(v.data) : std::get<double>(v.data);
            return Value(static_cast<std::int64_t>(std::ceil(d)));
        }
        if (name == "round") {
            if (e->args.size() != 1) throw RuntimeError("round expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            double d = std::holds_alternative<std::int64_t>(v.data) ? std::get<std::int64_t>(v.data) : std::get<double>(v.data);
            return Value(static_cast<std::int64_t>(std::round(d)));
        }
        if (name == "random") {
            static std::mt19937_64 rng(static_cast<unsigned long>(std::time(nullptr)));
            static std::uniform_real_distribution<double> dist(0.0, 1.0);
            return Value(dist(rng));
        }

        // Standard Filesystem functions: fs_read, fs_write, fs_exists, fs_remove, fs_list
        if (name == "fs_read") {
            if (e->args.size() != 1) throw RuntimeError("fs_read expects 1 argument", e->pos, "T3015");
            std::string p = eval(e->args[0]).str();
            std::ifstream f(p);
            if (!f) throw RuntimeError("fs_read: cannot open '" + p + "'", e->pos, "E1005");
            std::stringstream ss;
            ss << f.rdbuf();
            return Value(ss.str());
        }
        if (name == "fs_write") {
            if (e->args.size() != 2) throw RuntimeError("fs_write expects 2 arguments: (path, content)", e->pos, "T3015");
            std::string p = eval(e->args[0]).str();
            std::string content = eval(e->args[1]).str();
            std::ofstream f(p);
            if (!f) return Value(false);
            f << content;
            return Value(true);
        }
        if (name == "fs_exists") {
            if (e->args.size() != 1) throw RuntimeError("fs_exists expects 1 argument", e->pos, "T3015");
            std::string p = eval(e->args[0]).str();
            return Value(fs::exists(p));
        }
        if (name == "fs_remove") {
            if (e->args.size() != 1) throw RuntimeError("fs_remove expects 1 argument", e->pos, "T3015");
            std::string p = eval(e->args[0]).str();
            return Value(fs::remove(p));
        }
        if (name == "fs_list") {
            if (e->args.size() != 1) throw RuntimeError("fs_list expects 1 argument", e->pos, "T3015");
            std::string p = eval(e->args[0]).str();
            Value::Array files;
            if (fs::is_directory(p)) {
                for (const auto& entry : fs::directory_iterator(p)) {
                    files.push_back(Value(entry.path().filename().string()));
                }
            }
            return Value(std::move(files));
        }

        // OS & Time: env_get, env_set, time_now, exit
        if (name == "env_get") {
            if (e->args.size() != 1) throw RuntimeError("env_get expects 1 argument", e->pos, "T3015");
            const char* v = std::getenv(eval(e->args[0]).str().c_str());
            if (!v) return Value();
            return Value(std::string(v));
        }
        if (name == "env_set") {
            if (e->args.size() != 2) throw RuntimeError("env_set expects 2 arguments", e->pos, "T3015");
            std::string k = eval(e->args[0]).str();
            std::string v = eval(e->args[1]).str();
#ifdef _WIN32
            _putenv_s(k.c_str(), v.c_str());
#else
            setenv(k.c_str(), v.c_str(), 1);
#endif
            return Value();
        }
        if (name == "time_now") {
            return Value(static_cast<std::int64_t>(std::time(nullptr)));
        }
        if (name == "exit") {
            int code = e->args.empty() ? 0 : static_cast<int>(std::get<std::int64_t>(eval(e->args[0]).data));
            std::exit(code);
        }

        // Conversion functions: to_string, to_int, to_float
        if (name == "to_string") {
            if (e->args.size() != 1) throw RuntimeError("to_string expects 1 argument", e->pos, "T3015");
            return Value(eval(e->args[0]).str());
        }
        if (name == "to_int") {
            if (e->args.size() != 1) throw RuntimeError("to_int expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            if (auto p = std::get_if<std::int64_t>(&v.data)) return Value(*p);
            if (auto p = std::get_if<double>(&v.data)) return Value(static_cast<std::int64_t>(*p));
            if (auto p = std::get_if<std::string>(&v.data)) return Value(static_cast<std::int64_t>(std::stoll(*p)));
            return Value(static_cast<std::int64_t>(0));
        }
        if (name == "to_float") {
            if (e->args.size() != 1) throw RuntimeError("to_float expects 1 argument", e->pos, "T3015");
            auto v = eval(e->args[0]);
            if (auto p = std::get_if<double>(&v.data)) return Value(*p);
            if (auto p = std::get_if<std::int64_t>(&v.data)) return Value(static_cast<double>(*p));
            if (auto p = std::get_if<std::string>(&v.data)) return Value(std::stod(*p));
            return Value(0.0);
        }

        // Web write
        if (name == "web_write") {
            if (e->args.size() != 2) throw RuntimeError("web_write(path, content) expects 2 arguments", e->pos, "T3015");
            auto path = eval(e->args[0]).str();
            auto content = eval(e->args[1]).str();
            fs::path out = fs::path("dist") / path;
            if (out.string().find("..") != std::string::npos)
                throw RuntimeError("web_write path may not escape dist", e->pos, "T2105");
            fs::create_directories(out.parent_path());
            std::ofstream f(out, std::ios::binary);
            if (!f) throw RuntimeError("cannot write web file '" + out.string() + "'", e->pos, "T2105");
            f << content;
            return Value();
        }

        // Class Instantiation: Dog("Buddy", 3)
        auto cls_it = classes.find(name);
        if (cls_it != classes.end()) {
            std::vector<Value> arg_vals;
            for (auto& a : e->args) arg_vals.push_back(eval(a));
            return instantiate_class(cls_it->second, arg_vals);
        }

        // Struct Instantiation: User("Ajmal", 15)
        auto st = structs.find(name);
        if (st != structs.end()) {
            if (st->second.fields.size() != e->args.size())
                throw RuntimeError("wrong struct argument count for '" + name + "'", e->pos, "T3015");
            Value::Object o;
            for (std::size_t i = 0; i < e->args.size(); ++i) {
                o[st->second.fields[i]] = eval(e->args[i]);
            }
            return Value(std::move(o));
        }

        // Function call
        auto it = functions.find(name);
        if (it == functions.end()) {
            throw RuntimeError("undefined function '" + name + "'", e->pos, "T2001");
        }

        if (it->second.params.size() != e->args.size()) {
            throw RuntimeError("wrong argument count for '" + name + "'", e->pos, "T3015");
        }

        std::vector<Value> evaluated_args;
        for (auto& a : e->args) evaluated_args.push_back(eval(a));

        scopes.push_back({});
        for (std::size_t i = 0; i < evaluated_args.size(); ++i) {
            scopes.back()[it->second.params[i]] = {evaluated_args[i], true};
        }

        returning = false;
        return_value = {};
        exec_all(it->second.body);
        auto r = return_value;
        returning = false;
        scopes.pop_back();
        return r;
    }
    }

    throw RuntimeError("invalid expression", e ? e->pos : SourcePos{1, 1}, "T1008");
}

void Interpreter::exec(const StmtPtr& s) {
    if (!s || returning || breaking || continuing) return;

    if (s->kind == Stmt::Throw) {
        throw RuntimeError(eval(s->expr).str(), s->pos, "E1006");
    }

    if (s->kind == Stmt::Try) {
        try {
            exec_all(s->body);
        } catch (const std::exception& ex) {
            if (!s->catch_body.empty()) {
                scopes.push_back({});
                if (!s->catch_name.empty()) {
                    scopes.back()[s->catch_name] = {Value(std::string(ex.what())), true};
                }
                exec_all(s->catch_body);
                scopes.pop_back();
            } else {
                if (!s->finally_body.empty()) exec_all(s->finally_body);
                throw;
            }
        }
        if (!s->finally_body.empty()) exec_all(s->finally_body);
        return;
    }

    if (s->kind == Stmt::WebFile) {
        fs::path relative = s->web_path;
        if (relative.is_absolute() || s->web_path.empty() || s->web_path.find("..") != std::string::npos)
            throw RuntimeError("webfile path must be a non-empty relative path inside dist", s->pos, "T2105");
        auto out = fs::path("dist") / relative;
        fs::create_directories(out.parent_path());
        std::ofstream f(out, std::ios::binary);
        if (!f) throw RuntimeError("cannot write web file '" + out.string() + "'", s->pos, "T2105");
        for (const auto& part : s->web_parts) f << eval(part).str();
        return;
    }

    switch (s->kind) {
    case Stmt::Struct: {
        structs[s->name] = {s->fields, s->field_types};
        break;
    }

    case Stmt::Enum: {
        Value::Object values;
        for (const auto& v : s->enum_values) {
            values[v] = Value(s->name + "." + v);
        }
        if (!scopes.empty() && scopes.back().count(s->name))
            throw RuntimeError("binding already exists: " + s->name, s->pos, "T2002");
        if (!scopes.empty()) scopes.back()[s->name] = {Value(std::move(values)), false};
        enums[s->name] = {s->enum_values};
        break;
    }

    case Stmt::Trait: {
        TraitDef tr;
        tr.name = s->name;
        for (const auto& m : s->methods) {
            if (m->kind == Stmt::Function) {
                tr.methods[m->name] = {m->params, m->function_body, m->is_static, m->is_virtual, m->is_override};
            }
        }
        traits_map[s->name] = std::move(tr);
        break;
    }

    case Stmt::Class: {
        ClassDef cls;
        cls.name = s->name;
        cls.base_name = s->base_name;
        cls.traits = s->traits;
        for (const auto& m : s->methods) {
            if (m->kind == Stmt::Let) {
                cls.fields.push_back(m->name);
                cls.field_types.push_back(m->type_name);
            } else if (m->kind == Stmt::Function) {
                Function fn{m->params, m->function_body, m->is_static, m->is_virtual, m->is_override};
                if (m->is_static) cls.static_methods[m->name] = fn;
                else cls.methods[m->name] = fn;
            }
        }
        classes[s->name] = std::move(cls);
        break;
    }

    case Stmt::Match: {
        auto target = eval(s->match_expr);
        bool matched = false;

        for (const auto& mc : s->match_cases) {
            if (mc.is_wildcard) {
                exec_all(mc.body);
                matched = true;
                break;
            }

            // Tagged Option / Result match: some(x), none, ok(x), err(e)
            if (!mc.pattern_tag.empty()) {
                if (auto o = std::get_if<Value::Object>(&target.data)) {
                    auto it = o->find("tag");
                    if (it != o->end() && it->second.str() == mc.pattern_tag) {
                        scopes.push_back({});
                        if (!mc.pattern_var.empty()) {
                            auto val_it = o->find("value");
                            if (val_it != o->end()) {
                                scopes.back()[mc.pattern_var] = {val_it->second, true};
                            }
                        }
                        exec_all(mc.body);
                        scopes.pop_back();
                        matched = true;
                        break;
                    }
                }
                continue;
            }

            if (mc.pattern) {
                auto pat_val = eval(mc.pattern);
                if (binary("==", target, pat_val).truthy()) {
                    exec_all(mc.body);
                    matched = true;
                    break;
                }
            }
        }

        if (!matched && !s->match_default.empty()) {
            exec_all(s->match_default);
        }
        break;
    }

    case Stmt::Let: {
        auto val = s->expr ? eval(s->expr) : Value();
        if (!scopes.empty() && scopes.back().count(s->name))
            throw RuntimeError("binding already exists: " + s->name, s->pos, "T2002");
        if (!scopes.empty()) scopes.back()[s->name] = {std::move(val), s->mutable_binding};
        break;
    }

    case Stmt::Assign: {
        auto val = eval(s->expr);

        // Member assignment: obj.member = expr
        if (s->target && s->target->kind == Expr::Member) {
            if (s->target->object->kind == Expr::This || s->target->object->kind == Expr::Variable) {
                auto b = find(s->target->object->name);
                if (!b) throw RuntimeError("undefined object '" + s->target->object->name + "'", s->pos, "T2001");
                auto o = std::get_if<Value::Object>(&b->value.data);
                if (!o) throw RuntimeError("member assignment requires an object", s->pos, "T3021");
                (*o)[s->target->name] = val;
                break;
            }
        }

        // Index assignment: list[idx] = expr
        if (s->target && s->target->kind == Expr::Index) {
            auto arr_b = find(s->target->left->name);
            if (!arr_b) throw RuntimeError("undefined array '" + s->target->left->name + "'", s->pos, "T2001");
            auto arr = std::get_if<Value::Array>(&arr_b->value.data);
            if (!arr) throw RuntimeError("index assignment requires a list", s->pos, "T3012");
            auto idx_val = eval(s->target->index);
            auto idx = std::get_if<std::int64_t>(&idx_val.data);
            if (!idx || *idx < 0 || static_cast<std::size_t>(*idx) >= arr->size())
                throw RuntimeError("array index out of bounds", s->pos, "T3017");
            (*arr)[static_cast<std::size_t>(*idx)] = val;
            break;
        }

        auto b = find(s->name);
        if (!b) throw RuntimeError("undefined variable '" + s->name + "'", s->pos, "T2001");
        if (!b->mutable_binding)
            throw RuntimeError("cannot assign to immutable binding '" + s->name + "'", s->pos, "T3001");
        b->value = val;
        break;
    }

    case Stmt::Print: {
        for (std::size_t i = 0; i < s->print_args.size(); ++i) {
            if (i > 0) std::cout << ' ';
            std::cout << eval(s->print_args[i]).str();
        }
        std::cout << '\n';
        break;
    }

    case Stmt::ExprStmt:
        eval(s->expr);
        break;

    case Stmt::Function:
        functions[s->name] = {s->params, s->function_body, s->is_static, s->is_virtual, s->is_override};
        break;

    case Stmt::Return:
        return_value = s->expr ? eval(s->expr) : Value();
        returning = true;
        break;

    case Stmt::Break:
        breaking = true;
        break;

    case Stmt::Continue:
        continuing = true;
        break;

    case Stmt::If: {
        for (auto& b : s->branches) {
            if (eval(b.first).truthy()) {
                exec_all(b.second);
                return;
            }
        }
        exec_all(s->else_body);
        break;
    }

    case Stmt::While: {
        while (eval(s->expr).truthy()) {
            exec_all(s->body);
            if (returning) break;
            if (breaking) {
                breaking = false;
                break;
            }
            if (continuing) {
                continuing = false;
                continue;
            }
        }
        break;
    }

    case Stmt::For: {
        if (s->for_end) {
            auto a = eval(s->for_start);
            auto b = eval(s->for_end);
            auto x = std::get_if<std::int64_t>(&a.data);
            auto y = std::get_if<std::int64_t>(&b.data);
            if (!x || !y) throw RuntimeError("for range requires int bounds", s->pos, "T3005");
            scopes.push_back({});
            scopes.back()[s->name] = {Value(*x), true};
            for (std::int64_t n = *x; n <= *y; ++n) {
                scopes.back()[s->name].value = Value(n);
                exec_all(s->body);
                if (returning) break;
                if (breaking) {
                    breaking = false;
                    break;
                }
                if (continuing) {
                    continuing = false;
                }
            }
            scopes.pop_back();
        } else {
            // Collection for item in list
            auto coll = eval(s->for_start);
            if (auto arr = std::get_if<Value::Array>(&coll.data)) {
                scopes.push_back({});
                for (const auto& item : *arr) {
                    scopes.back()[s->name] = {item, true};
                    exec_all(s->body);
                    if (returning) break;
                    if (breaking) {
                        breaking = false;
                        break;
                    }
                    if (continuing) {
                        continuing = false;
                    }
                }
                scopes.pop_back();
            }
        }
        break;
    }

    default:
        break;
    }
}

void Interpreter::exec_all(const std::vector<StmtPtr>& body) {
    for (const auto& s : body) {
        exec(s);
        if (returning || breaking || continuing) break;
    }
}

Value Interpreter::eval_expr(const ExprPtr& expr) {
    return eval(expr);
}

void Interpreter::run(const Program& p) {
    scopes.clear();
    functions.clear();
    structs.clear();
    enums.clear();
    classes.clear();
    traits_map.clear();
    returning = breaking = continuing = false;
    scopes.push_back({});
    exec_all(p.statements);
}

} // namespace ternet
