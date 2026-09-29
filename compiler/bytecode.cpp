#include "bytecode.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace ternet::bytecode {
namespace {

struct LoopContext {
    std::size_t continue_target = 0;
    std::vector<std::size_t> breaks;
    std::vector<std::size_t> continues;
};

struct Compiler {
    Chunk c;
    std::vector<LoopContext> loops;
    std::unordered_map<std::string, int> function_ids;
    std::unordered_map<std::string, std::vector<std::string>> struct_fields;
    std::unordered_map<std::string, std::vector<std::string>> enum_values;
    std::unordered_map<std::string, ClassInfo> class_defs;
    bool in_function = false;

    int constant(Value v) {
        c.constants.push_back(std::move(v));
        return static_cast<int>(c.constants.size() - 1);
    }

    int name(const std::string& n) {
        for (std::size_t i = 0; i < c.names.size(); ++i) {
            if (c.names[i] == n) return static_cast<int>(i);
        }
        c.names.push_back(n);
        return static_cast<int>(c.names.size() - 1);
    }

    std::size_t emit(Op op, int operand = 0) {
        c.code.push_back({op, operand});
        return c.code.size() - 1;
    }

    void patch(std::size_t at, std::size_t target) {
        if (target > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
            throw CompileError("T6000: bytecode too large");
        c.code[at].operand = static_cast<std::int32_t>(target);
    }

    void expr(const ExprPtr& e) {
        if (!e) throw CompileError("T2000: null expression");

        switch (e->kind) {
        case Expr::Literal:
            emit(Op::Const, constant(e->literal));
            return;

        case Expr::Variable:
            emit(Op::Load, name(e->name));
            return;

        case Expr::This:
            emit(Op::Load, name("this"));
            return;

        case Expr::Super:
            emit(Op::Load, name("this"));
            return;

        case Expr::Array:
            for (const auto& item : e->args) expr(item);
            emit(Op::MakeArray, static_cast<int>(e->args.size()));
            return;

        case Expr::Tuple:
            for (const auto& item : e->args) expr(item);
            emit(Op::MakeTuple, static_cast<int>(e->args.size()));
            return;

        case Expr::Index:
            expr(e->left);
            expr(e->index);
            emit(Op::Index);
            return;

        case Expr::Member:
            expr(e->object);
            emit(Op::GetMember, name(e->name));
            return;

        case Expr::Unary:
            expr(e->right);
            if (e->op == "-") emit(Op::Neg);
            else if (e->op == "!") emit(Op::Not);
            else if (e->op == "+") { /* no-op */ }
            else if (e->op == "~") {
                emit(Op::Const, constant(Value(static_cast<std::int64_t>(-1))));
                emit(Op::BitXor);
            } else throw CompileError("T3002: unsupported unary operator '" + e->op + "'");
            return;

        case Expr::Call: {
            // Method Call: obj.method(args)
            if (e->left && e->left->kind == Expr::Member) {
                expr(e->left->object);
                for (const auto& arg : e->args) expr(arg);
                emit(Op::CallMethod, name(e->left->name));
                return;
            }

            if (!e->left || e->left->kind != Expr::Variable)
                throw CompileError("T3014: call target must be a function or identifier");

            const auto builtin = e->left->name;

            // Option / Result constructors
            auto emit_tagged = [&](const std::string& tag, bool has_value) {
                emit(Op::Const, constant(Value("tag")));
                emit(Op::Const, constant(Value(tag)));
                if (has_value) {
                    emit(Op::Const, constant(Value("value")));
                    expr(e->args[0]);
                    emit(Op::MakeObject, 2);
                } else {
                    emit(Op::MakeObject, 1);
                }
            };

            if (builtin == "some" || builtin == "ok" || builtin == "err") {
                if (e->args.size() != 1) throw CompileError("T3034: " + builtin + "() expects one value");
                emit_tagged(builtin, true);
                return;
            }
            if (builtin == "none") {
                if (!e->args.empty()) throw CompileError("T3034: none() expects no arguments");
                emit_tagged("none", false);
                return;
            }
            if (builtin == "is_some" || builtin == "is_none") {
                if (e->args.size() != 1) throw CompileError("T3036: " + builtin + "() expects one value");
                expr(e->args[0]);
                emit(Op::GetMember, name("tag"));
                emit(Op::Const, constant(Value(builtin == "is_some" ? "some" : "none")));
                emit(Op::Eq);
                return;
            }
            if (builtin == "is_ok" || builtin == "is_err") {
                if (e->args.size() != 1) throw CompileError("T3036: " + builtin + "() expects one value");
                expr(e->args[0]);
                emit(Op::GetMember, name("tag"));
                emit(Op::Const, constant(Value(builtin == "is_ok" ? "ok" : "err")));
                emit(Op::Eq);
                return;
            }
            if (builtin == "unwrap") {
                if (e->args.size() != 1) throw CompileError("T3037: unwrap() expects 1 argument");
                expr(e->args[0]);
                emit(Op::GetMember, name("value"));
                return;
            }
            if (builtin == "unwrap_or") {
                if (e->args.size() != 2) throw CompileError("T3037: unwrap_or() expects 2 arguments");
                expr(e->args[0]);
                emit(Op::GetMember, name("tag"));
                emit(Op::Const, constant(Value("some")));
                emit(Op::Eq);
                const auto jf = emit(Op::JumpIfFalse);
                expr(e->args[0]);
                emit(Op::GetMember, name("value"));
                const auto jend = emit(Op::Jump);
                const auto fallback = c.code.size();
                expr(e->args[1]);
                const auto end = c.code.size();
                patch(jf, fallback);
                patch(jend, end);
                return;
            }

            // Class Instantiation in Bytecode: Dog("Buddy", 3)
            auto cls_it = class_defs.find(builtin);
            if (cls_it != class_defs.end()) {
                emit(Op::Const, constant(Value("__class__")));
                emit(Op::Const, constant(Value(builtin)));
                for (const auto& field_name : cls_it->second.fields) {
                    emit(Op::Const, constant(Value(field_name)));
                    emit(Op::Const, constant(Value{}));
                }
                int field_count = 1 + static_cast<int>(cls_it->second.fields.size());
                emit(Op::MakeObject, field_count);

                // If constructor init exists, call it on instance
                std::string init_fn_name = builtin + ".init";
                auto init_it = function_ids.find(init_fn_name);
                if (init_it != function_ids.end()) {
                    emit(Op::Dup);
                    emit(Op::Store, name("this"));
                    for (const auto& a : e->args) expr(a);
                    emit(Op::Call, init_it->second);
                    emit(Op::Pop);
                    emit(Op::Load, name("this"));
                }
                return;
            }

            // Struct Instantiation
            const auto st = struct_fields.find(builtin);
            if (st != struct_fields.end()) {
                if (st->second.size() != e->args.size())
                    throw CompileError("T3015: wrong field count for struct '" + builtin + "'");
                for (std::size_t i = 0; i < e->args.size(); ++i) {
                    emit(Op::Const, constant(Value(st->second[i])));
                    expr(e->args[i]);
                }
                emit(Op::MakeObject, static_cast<int>(e->args.size()));
                return;
            }

            // Function call
            const auto it = function_ids.find(builtin);
            if (it != function_ids.end()) {
                const auto& fn = c.functions[static_cast<std::size_t>(it->second)];
                if (fn.params.size() != e->args.size())
                    throw CompileError("T3015: wrong argument count for '" + builtin + "'");
                for (const auto& a : e->args) expr(a);
                emit(Op::Call, it->second);
                return;
            }

            // Built-in functions directly handled
            for (const auto& a : e->args) expr(a);
            emit(Op::Load, name(builtin));
            emit(Op::CallMethod, name("call"));
            return;
        }

        case Expr::Binary:
            if (e->op == "&&") {
                expr(e->left);
                emit(Op::Dup);
                const auto jf = emit(Op::JumpIfFalse);
                emit(Op::Pop);
                expr(e->right);
                patch(jf, c.code.size());
                return;
            }

            if (e->op == "||") {
                expr(e->left);
                emit(Op::Dup);
                const auto jt = emit(Op::JumpIfTrue);
                emit(Op::Pop);
                expr(e->right);
                patch(jt, c.code.size());
                return;
            }

            expr(e->left);
            expr(e->right);
            if (e->op == "+") emit(Op::Add);
            else if (e->op == "-") emit(Op::Sub);
            else if (e->op == "*") emit(Op::Mul);
            else if (e->op == "/") emit(Op::Div);
            else if (e->op == "%") emit(Op::Mod);
            else if (e->op == "&") emit(Op::BitAnd);
            else if (e->op == "|") emit(Op::BitOr);
            else if (e->op == "^") emit(Op::BitXor);
            else if (e->op == "<<") emit(Op::ShiftLeft);
            else if (e->op == ">>") emit(Op::ShiftRight);
            else if (e->op == "==") emit(Op::Eq);
            else if (e->op == "!=") emit(Op::Ne);
            else if (e->op == "<") emit(Op::Lt);
            else if (e->op == "<=") emit(Op::Le);
            else if (e->op == ">") emit(Op::Gt);
            else if (e->op == ">=") emit(Op::Ge);
            else throw CompileError("T3002: unsupported binary operator '" + e->op + "'");
            return;

        default:
            throw CompileError("T2100: expression is not supported by bytecode compiler");
        }
    }

    void statements(const std::vector<StmtPtr>& body) {
        for (const auto& s : body) stmt(s);
    }

    void stmt(const StmtPtr& s) {
        if (!s) return;

        switch (s->kind) {
        case Stmt::Enum: {
            for (const auto& value : s->enum_values) {
                emit(Op::Const, constant(Value(value)));
                emit(Op::Const, constant(Value(s->name + "." + value)));
            }
            emit(Op::MakeObject, static_cast<int>(s->enum_values.size()));
            emit(Op::Store, name(s->name));
            return;
        }

        case Stmt::Let:
            if (s->expr) expr(s->expr);
            else emit(Op::Const, constant(Value{}));
            emit(Op::Store, name(s->name));
            return;

        case Stmt::Assign:
            if (s->target && s->target->kind == Expr::Member &&
                s->target->object && (s->target->object->kind == Expr::Variable || s->target->object->kind == Expr::This)) {
                expr(s->target->object);
                expr(s->expr);
                emit(Op::SetMember, name(s->target->name));
                emit(Op::Store, name(s->target->object->name));
            } else if (s->target && s->target->kind == Expr::Index) {
                expr(s->target->left);
                expr(s->target->index);
                expr(s->expr);
                emit(Op::SetIndex);
                if (s->target->left->kind == Expr::Variable) {
                    emit(Op::Store, name(s->target->left->name));
                } else {
                    emit(Op::Pop);
                }
            } else {
                expr(s->expr);
                emit(Op::Store, name(s->name));
            }
            return;

        case Stmt::Print:
            for (const auto& a : s->print_args) {
                expr(a);
                emit(Op::Print);
            }
            return;

        case Stmt::ExprStmt:
            expr(s->expr);
            emit(Op::Pop);
            return;

        case Stmt::Return:
            if (!in_function) throw CompileError("T3016: return outside function");
            if (s->expr) expr(s->expr);
            else emit(Op::Const, constant(Value{}));
            emit(Op::Return);
            return;

        case Stmt::Match: {
            expr(s->match_expr);
            std::vector<std::size_t> exits;
            for (const auto& mc : s->match_cases) {
                if (mc.is_wildcard) {
                    emit(Op::Pop);
                    statements(mc.body);
                    for (const auto j : exits) patch(j, c.code.size());
                    return;
                }
                if (!mc.pattern_tag.empty()) {
                    emit(Op::Dup);
                    emit(Op::GetMember, name("tag"));
                    emit(Op::Const, constant(Value(mc.pattern_tag)));
                    emit(Op::Eq);
                    const auto jf = emit(Op::JumpIfFalse);
                    if (!mc.pattern_var.empty()) {
                        emit(Op::Dup);
                        emit(Op::GetMember, name("value"));
                        emit(Op::Store, name(mc.pattern_var));
                    }
                    statements(mc.body);
                    emit(Op::Pop);
                    const auto jend = emit(Op::Jump);
                    exits.push_back(jend);
                    patch(jf, c.code.size());
                    continue;
                }
                if (mc.pattern) {
                    emit(Op::Dup);
                    expr(mc.pattern);
                    emit(Op::Eq);
                    const auto jf = emit(Op::JumpIfFalse);
                    statements(mc.body);
                    emit(Op::Pop);
                    const auto jend = emit(Op::Jump);
                    exits.push_back(jend);
                    patch(jf, c.code.size());
                }
            }
            emit(Op::Pop);
            statements(s->match_default);
            for (const auto j : exits) patch(j, c.code.size());
            return;
        }

        case Stmt::If: {
            std::vector<std::size_t> exits;
            for (const auto& branch : s->branches) {
                expr(branch.first);
                const auto jf = emit(Op::JumpIfFalse);
                statements(branch.second);
                const auto jend = emit(Op::Jump);
                exits.push_back(jend);
                patch(jf, c.code.size());
            }
            statements(s->else_body);
            for (const auto j : exits) patch(j, c.code.size());
            return;
        }

        case Stmt::While: {
            const auto start = c.code.size();
            expr(s->expr);
            const auto jf = emit(Op::JumpIfFalse);

            loops.push_back({start, {}, {}});
            statements(s->body);
            emit(Op::Jump, static_cast<int>(start));

            const auto end = c.code.size();
            patch(jf, end);
            for (const auto br : loops.back().breaks) patch(br, end);
            for (const auto co : loops.back().continues) patch(co, start);
            loops.pop_back();
            return;
        }

        case Stmt::For: {
            if (s->for_end) {
                expr(s->for_start);
                emit(Op::Store, name(s->name));
                const int end_name = name("__ternet_for_end_" + std::to_string(c.code.size()));
                expr(s->for_end);
                emit(Op::Store, end_name);

                const auto start = c.code.size();
                emit(Op::Load, name(s->name));
                emit(Op::Load, end_name);
                emit(Op::Le);
                const auto jf = emit(Op::JumpIfFalse);

                loops.push_back({start, {}, {}});
                statements(s->body);
                const auto continue_target = c.code.size();
                loops.back().continue_target = continue_target;
                emit(Op::Load, name(s->name));
                emit(Op::Const, constant(Value(static_cast<std::int64_t>(1))));
                emit(Op::Add);
                emit(Op::Store, name(s->name));
                emit(Op::Jump, static_cast<int>(start));

                const auto end = c.code.size();
                patch(jf, end);
                for (const auto br : loops.back().breaks) patch(br, end);
                for (const auto co : loops.back().continues) patch(co, continue_target);
                loops.pop_back();
            } else {
                // For-in collection
                const int arr_name = name("__ternet_for_arr_" + std::to_string(c.code.size()));
                const int idx_name = name("__ternet_for_idx_" + std::to_string(c.code.size()));
                expr(s->for_start);
                emit(Op::Store, arr_name);
                emit(Op::Const, constant(Value(static_cast<std::int64_t>(0))));
                emit(Op::Store, idx_name);

                const auto start = c.code.size();
                emit(Op::Load, idx_name);
                emit(Op::Load, arr_name);
                emit(Op::GetMember, name("len"));
                emit(Op::Lt);
                const auto jf = emit(Op::JumpIfFalse);

                emit(Op::Load, arr_name);
                emit(Op::Load, idx_name);
                emit(Op::Index);
                emit(Op::Store, name(s->name));

                loops.push_back({start, {}, {}});
                statements(s->body);
                const auto continue_target = c.code.size();
                loops.back().continue_target = continue_target;
                emit(Op::Load, idx_name);
                emit(Op::Const, constant(Value(static_cast<std::int64_t>(1))));
                emit(Op::Add);
                emit(Op::Store, idx_name);
                emit(Op::Jump, static_cast<int>(start));

                const auto end = c.code.size();
                patch(jf, end);
                for (const auto br : loops.back().breaks) patch(br, end);
                for (const auto co : loops.back().continues) patch(co, continue_target);
                loops.pop_back();
            }
            return;
        }

        case Stmt::Break:
            if (loops.empty()) throw CompileError("T2101: break outside loop");
            loops.back().breaks.push_back(emit(Op::Jump));
            return;

        case Stmt::Continue:
            if (loops.empty()) throw CompileError("T2102: continue outside loop");
            emit(Op::Jump, static_cast<int>(loops.back().continue_target));
            return;

        case Stmt::Throw:
            expr(s->expr);
            emit(Op::Throw);
            return;

        case Stmt::Try: {
            const auto catch_jump = emit(Op::PushCatch);
            statements(s->body);
            emit(Op::PopCatch);
            const auto end_jump = emit(Op::Jump);

            const auto catch_start = c.code.size();
            patch(catch_jump, catch_start);
            if (!s->catch_name.empty()) {
                emit(Op::Store, name(s->catch_name));
            } else {
                emit(Op::Pop);
            }
            statements(s->catch_body);

            const auto end = c.code.size();
            patch(end_jump, end);
            statements(s->finally_body);
            return;
        }

        case Stmt::WebFile: {
            emit(Op::Const, constant(Value(std::string{})));
            for (const auto& part : s->web_parts) {
                expr(part);
                emit(Op::Add);
            }
            emit(Op::WebWrite, constant(Value(s->web_path)));
            return;
        }

        case Stmt::Struct:
        case Stmt::Trait:
        case Stmt::Class:
        case Stmt::Function:
            return;

        default:
            throw CompileError("T2100: statement is not supported by bytecode compiler");
        }
    }
};

} // namespace

Chunk compile(const Program& program) {
    Compiler x;

    // Collect structs, enums, and classes
    for (const auto& s : program.statements) {
        if (!s) continue;
        if (s->kind == Stmt::Struct) x.struct_fields[s->name] = s->fields;
        if (s->kind == Stmt::Enum) x.enum_values[s->name] = s->enum_values;
        if (s->kind == Stmt::Class) {
            ClassInfo ci;
            ci.name = s->name;
            ci.base_name = s->base_name;
            for (const auto& m : s->methods) {
                if (m->kind == Stmt::Let) ci.fields.push_back(m->name);
                else if (m->kind == Stmt::Function) ci.methods.push_back(m->name);
            }
            x.class_defs[s->name] = ci;
            x.c.classes.push_back(ci);
        }
    }

    // Collect top-level functions
    for (const auto& s : program.statements) {
        if (!s || s->kind != Stmt::Function) continue;
        const int id = static_cast<int>(x.c.functions.size());
        x.function_ids.emplace(s->name, id);
        FunctionInfo info;
        info.name = s->name;
        info.params = s->params;
        x.c.functions.push_back(std::move(info));
    }

    // Collect class methods
    for (const auto& s : program.statements) {
        if (!s || s->kind != Stmt::Class) continue;
        for (const auto& m : s->methods) {
            if (m->kind != Stmt::Function) continue;
            std::string full_name = s->name + "." + m->name;
            const int id = static_cast<int>(x.c.functions.size());
            x.function_ids.emplace(full_name, id);
            FunctionInfo info;
            info.name = full_name;
            info.params = m->params;
            x.c.functions.push_back(std::move(info));
        }
    }

    // Compile top-level statements
    for (const auto& s : program.statements) {
        if (s && s->kind != Stmt::Function && s->kind != Stmt::Class && s->kind != Stmt::Trait) {
            x.stmt(s);
        }
    }
    x.emit(Op::Halt);

    // Compile top-level function bodies
    for (const auto& s : program.statements) {
        if (!s || s->kind != Stmt::Function) continue;
        const int id = x.function_ids.at(s->name);
        x.c.functions[static_cast<std::size_t>(id)].entry = static_cast<std::int32_t>(x.c.code.size());
        x.in_function = true;
        x.loops.clear();
        x.statements(s->function_body);
        x.emit(Op::Const, x.constant(Value{}));
        x.emit(Op::Return);
        x.in_function = false;
    }

    // Compile class method bodies
    for (const auto& s : program.statements) {
        if (!s || s->kind != Stmt::Class) continue;
        for (const auto& m : s->methods) {
            if (m->kind != Stmt::Function) continue;
            std::string full_name = s->name + "." + m->name;
            const int id = x.function_ids.at(full_name);
            x.c.functions[static_cast<std::size_t>(id)].entry = static_cast<std::int32_t>(x.c.code.size());
            x.in_function = true;
            x.loops.clear();
            x.statements(m->function_body);
            x.emit(Op::Const, x.constant(Value{}));
            x.emit(Op::Return);
            x.in_function = false;
        }
    }

    verify(x.c);
    return x.c;
}

void verify(const Chunk& c) {
    if (c.code.empty()) throw CompileError("T6001: empty bytecode");

    for (std::size_t i = 0; i < c.code.size(); ++i) {
        const auto& ins = c.code[i];
        auto valid = [](int n, std::size_t size) {
            return n >= 0 && static_cast<std::size_t>(n) < size;
        };

        switch (ins.op) {
        case Op::Const:
        case Op::WebWrite:
            if (!valid(ins.operand, c.constants.size()))
                throw CompileError("T6002: invalid constant index");
            break;
        case Op::Load:
        case Op::Store:
        case Op::GetMember:
        case Op::SetMember:
        case Op::CallMethod:
            if (!valid(ins.operand, c.names.size()))
                throw CompileError("T6003: invalid name index");
            break;
        case Op::MakeArray:
        case Op::MakeTuple:
        case Op::MakeObject:
            if (ins.operand < 0) throw CompileError("T6005: invalid collection size");
            break;
        case Op::Call:
            if (!valid(ins.operand, c.functions.size()))
                throw CompileError("T6022: invalid function index");
            if (c.functions[static_cast<std::size_t>(ins.operand)].entry < 0 ||
                !valid(c.functions[static_cast<std::size_t>(ins.operand)].entry, c.code.size()))
                throw CompileError("T6023: invalid function entry");
            break;
        case Op::Jump:
        case Op::JumpIfFalse:
        case Op::JumpIfTrue:
        case Op::PushCatch:
            if (!valid(ins.operand, c.code.size()))
                throw CompileError("T6004: invalid jump target");
            break;
        default:
            break;
        }
    }
}

void write(const Chunk& c, const std::string& path) {
    verify(c);
    std::ofstream f(path, std::ios::binary);
    if (!f) throw CompileError("T6006: cannot open bytecode output '" + path + "'");

    const std::uint32_t magic = 0x544E4233;
    const std::uint32_t nc = static_cast<std::uint32_t>(c.constants.size());
    const std::uint32_t nn = static_cast<std::uint32_t>(c.names.size());
    const std::uint32_t ni = static_cast<std::uint32_t>(c.code.size());
    const std::uint32_t nf = static_cast<std::uint32_t>(c.functions.size());

    f.write(reinterpret_cast<const char*>(&magic), 4);
    f.write(reinterpret_cast<const char*>(&nc), 4);
    f.write(reinterpret_cast<const char*>(&nn), 4);
    f.write(reinterpret_cast<const char*>(&ni), 4);
    f.write(reinterpret_cast<const char*>(&nf), 4);

    for (const auto& v : c.constants) {
        std::uint8_t tag = 0;
        if (std::holds_alternative<bool>(v.data)) tag = 1;
        else if (std::holds_alternative<std::int64_t>(v.data)) tag = 2;
        else if (std::holds_alternative<double>(v.data)) tag = 3;
        else if (std::holds_alternative<std::string>(v.data)) tag = 4;
        else if (!std::holds_alternative<std::monostate>(v.data))
            throw CompileError("T6014: unsupported constant type");

        f.write(reinterpret_cast<const char*>(&tag), 1);
        if (tag == 1) {
            const auto x = std::get<bool>(v.data);
            f.write(reinterpret_cast<const char*>(&x), 1);
        } else if (tag == 2) {
            const auto x = std::get<std::int64_t>(v.data);
            f.write(reinterpret_cast<const char*>(&x), 8);
        } else if (tag == 3) {
            const auto x = std::get<double>(v.data);
            f.write(reinterpret_cast<const char*>(&x), 8);
        } else if (tag == 4) {
            const auto& s = std::get<std::string>(v.data);
            const std::uint32_t n = static_cast<std::uint32_t>(s.size());
            f.write(reinterpret_cast<const char*>(&n), 4);
            f.write(s.data(), static_cast<std::streamsize>(n));
        }
    }

    for (const auto& n : c.names) {
        const std::uint32_t len = static_cast<std::uint32_t>(n.size());
        f.write(reinterpret_cast<const char*>(&len), 4);
        f.write(n.data(), static_cast<std::streamsize>(len));
    }

    for (const auto& fn : c.functions) {
        const std::uint32_t nl = static_cast<std::uint32_t>(fn.name.size());
        f.write(reinterpret_cast<const char*>(&nl), 4);
        f.write(fn.name.data(), static_cast<std::streamsize>(nl));
        const std::uint32_t np = static_cast<std::uint32_t>(fn.params.size());
        f.write(reinterpret_cast<const char*>(&np), 4);
        for (const auto& p : fn.params) {
            const std::uint32_t pl = static_cast<std::uint32_t>(p.size());
            f.write(reinterpret_cast<const char*>(&pl), 4);
            f.write(p.data(), static_cast<std::streamsize>(pl));
        }
        f.write(reinterpret_cast<const char*>(&fn.entry), 4);
    }

    for (const auto& ins : c.code) {
        const auto op = static_cast<std::uint8_t>(ins.op);
        f.write(reinterpret_cast<const char*>(&op), 1);
        f.write(reinterpret_cast<const char*>(&ins.operand), 4);
    }

    if (!f) throw CompileError("T6015: failed writing bytecode");
}

Chunk read(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw CompileError("T6007: cannot open bytecode '" + path + "'");

    std::uint32_t magic = 0, nc = 0, nn = 0, ni = 0, nf = 0;
    f.read(reinterpret_cast<char*>(&magic), 4);
    f.read(reinterpret_cast<char*>(&nc), 4);
    f.read(reinterpret_cast<char*>(&nn), 4);
    f.read(reinterpret_cast<char*>(&ni), 4);
    f.read(reinterpret_cast<char*>(&nf), 4);

    if (!f) throw CompileError("T6011: truncated bytecode header");
    if (magic != 0x544E4233)
        throw CompileError("T6008: invalid or unsupported Ternet bytecode");

    Chunk c;
    for (std::uint32_t i = 0; i < nc; ++i) {
        std::uint8_t tag = 0;
        f.read(reinterpret_cast<char*>(&tag), 1);
        if (tag == 0) c.constants.emplace_back(Value{});
        else if (tag == 1) {
            std::uint8_t x = 0;
            f.read(reinterpret_cast<char*>(&x), 1);
            c.constants.emplace_back(x != 0);
        } else if (tag == 2) {
            std::int64_t x = 0;
            f.read(reinterpret_cast<char*>(&x), 8);
            c.constants.emplace_back(x);
        } else if (tag == 3) {
            double x = 0;
            f.read(reinterpret_cast<char*>(&x), 8);
            c.constants.emplace_back(x);
        } else if (tag == 4) {
            std::uint32_t n = 0;
            f.read(reinterpret_cast<char*>(&n), 4);
            if (n > 100000000) throw CompileError("T6016: oversized string constant");
            std::string s(n, '\0');
            f.read(s.data(), static_cast<std::streamsize>(n));
            c.constants.emplace_back(std::move(s));
        } else {
            throw CompileError("T6017: invalid constant tag");
        }
    }

    for (std::uint32_t i = 0; i < nn; ++i) {
        std::uint32_t n = 0;
        f.read(reinterpret_cast<char*>(&n), 4);
        std::string s(n, '\0');
        f.read(s.data(), static_cast<std::streamsize>(n));
        c.names.push_back(std::move(s));
    }

    for (std::uint32_t i = 0; i < nf; ++i) {
        std::uint32_t nl = 0;
        f.read(reinterpret_cast<char*>(&nl), 4);
        std::string name(nl, '\0');
        f.read(name.data(), static_cast<std::streamsize>(nl));
        std::uint32_t np = 0;
        f.read(reinterpret_cast<char*>(&np), 4);
        FunctionInfo fn;
        fn.name = std::move(name);
        for (std::uint32_t p = 0; p < np; ++p) {
            std::uint32_t pl = 0;
            f.read(reinterpret_cast<char*>(&pl), 4);
            std::string param(pl, '\0');
            f.read(param.data(), static_cast<std::streamsize>(pl));
            fn.params.push_back(std::move(param));
        }
        f.read(reinterpret_cast<char*>(&fn.entry), 4);
        c.functions.push_back(std::move(fn));
    }

    for (std::uint32_t i = 0; i < ni; ++i) {
        std::uint8_t op = 0;
        std::int32_t operand = 0;
        f.read(reinterpret_cast<char*>(&op), 1);
        f.read(reinterpret_cast<char*>(&operand), 4);
        c.code.push_back({static_cast<Op>(op), operand});
    }

    if (!f) throw CompileError("T6011: truncated bytecode");
    verify(c);
    return c;
}

Value execute(const Chunk& c) {
    verify(c);

    struct Frame {
        std::size_t return_ip;
        std::unordered_map<std::string, Value> locals;
    };

    struct CatchFrame {
        std::size_t handler_ip;
        std::size_t stack_size;
        std::size_t call_depth;
    };

    std::vector<Value> stack;
    std::unordered_map<std::string, Value> vars;
    std::vector<Frame> frames;
    std::vector<CatchFrame> catch_stack;

    auto pop = [&]() -> Value {
        if (stack.empty()) throw CompileError("T6012: bytecode stack underflow");
        auto v = stack.back();
        stack.pop_back();
        return v;
    };

    auto num = [](const Value& v) -> double {
        if (auto p = std::get_if<std::int64_t>(&v.data)) return static_cast<double>(*p);
        if (auto p = std::get_if<double>(&v.data)) return *p;
        throw CompileError("T3002: numeric operator requires numbers");
    };

    Value result;
    std::size_t ip = 0;

    while (ip < c.code.size()) {
        const auto ins = c.code[ip++];

        switch (ins.op) {
        case Op::Halt:
            return result;

        case Op::Const:
            stack.push_back(c.constants[static_cast<std::size_t>(ins.operand)]);
            break;

        case Op::Load: {
            const auto& key = c.names[static_cast<std::size_t>(ins.operand)];
            if (!frames.empty()) {
                const auto lit = frames.back().locals.find(key);
                if (lit != frames.back().locals.end()) {
                    stack.push_back(lit->second);
                    break;
                }
            }
            const auto it = vars.find(key);
            if (it != vars.end()) {
                stack.push_back(it->second);
                break;
            }
            // Check built-in functions
            if (key == "print" || key == "tnprint" || key == "len" || key == "input") {
                stack.push_back(Value(key));
                break;
            }
            throw CompileError("T2001: undefined variable '" + key + "'");
        }

        case Op::Store: {
            auto value = pop();
            const auto& key = c.names[static_cast<std::size_t>(ins.operand)];
            if (!frames.empty()) frames.back().locals[key] = std::move(value);
            else vars[key] = std::move(value);
            break;
        }

        case Op::Pop:
            pop();
            break;

        case Op::Dup:
            if (stack.empty()) throw CompileError("T6012: bytecode stack underflow");
            stack.push_back(stack.back());
            break;

        case Op::MakeArray: {
            const auto count = static_cast<std::size_t>(ins.operand);
            if (count > stack.size()) throw CompileError("T6012: bytecode stack underflow");
            Value::Array a(count);
            for (std::size_t i = count; i-- > 0;) a[i] = pop();
            stack.emplace_back(std::move(a));
            break;
        }

        case Op::MakeTuple: {
            const auto count = static_cast<std::size_t>(ins.operand);
            if (count > stack.size()) throw CompileError("T6012: bytecode stack underflow");
            Value::Tuple t;
            t.items.resize(count);
            for (std::size_t i = count; i-- > 0;) t.items[i] = pop();
            stack.emplace_back(std::move(t));
            break;
        }

        case Op::MakeObject: {
            const auto count = static_cast<std::size_t>(ins.operand);
            if (stack.size() < count * 2) throw CompileError("T6012: bytecode stack underflow");
            Value::Object object;
            for (std::size_t i = 0; i < count; ++i) {
                auto value = pop();
                auto key = pop();
                if (!std::holds_alternative<std::string>(key.data))
                    throw CompileError("T3018: object field name must be string");
                object[std::get<std::string>(key.data)] = std::move(value);
            }
            stack.emplace_back(std::move(object));
            break;
        }

        case Op::Index: {
            const auto index = pop();
            const auto coll = pop();
            if (auto a = std::get_if<Value::Array>(&coll.data)) {
                auto p = std::get_if<std::int64_t>(&index.data);
                if (!p || *p < 0 || static_cast<std::size_t>(*p) >= a->size())
                    throw CompileError("T3017: array index out of bounds");
                stack.push_back((*a)[static_cast<std::size_t>(*p)]);
            } else if (auto t = std::get_if<Value::Tuple>(&coll.data)) {
                auto p = std::get_if<std::int64_t>(&index.data);
                if (!p || *p < 0 || static_cast<std::size_t>(*p) >= t->items.size())
                    throw CompileError("T3017: tuple index out of bounds");
                stack.push_back(t->items[static_cast<std::size_t>(*p)]);
            } else if (auto s = std::get_if<std::string>(&coll.data)) {
                auto p = std::get_if<std::int64_t>(&index.data);
                if (!p || *p < 0 || static_cast<std::size_t>(*p) >= s->size())
                    throw CompileError("T3017: string index out of bounds");
                stack.push_back(std::string(1, (*s)[static_cast<std::size_t>(*p)]));
            } else {
                throw CompileError("T3012: index requires collection");
            }
            break;
        }

        case Op::SetIndex: {
            auto val = pop();
            auto idx = pop();
            auto arr_val = pop();
            auto arr = std::get_if<Value::Array>(&arr_val.data);
            auto p = std::get_if<std::int64_t>(&idx.data);
            if (!arr || !p || *p < 0 || static_cast<std::size_t>(*p) >= arr->size())
                throw CompileError("T3017: array index out of bounds");
            (*arr)[static_cast<std::size_t>(*p)] = val;
            stack.push_back(std::move(arr_val));
            break;
        }

        case Op::GetMember: {
            const auto object = pop();
            const auto& key = c.names[static_cast<std::size_t>(ins.operand)];

            if (auto t = std::get_if<Value::Tuple>(&object.data)) {
                std::size_t idx = static_cast<std::size_t>(std::stoul(key));
                if (idx >= t->items.size()) throw CompileError("T3017: tuple index out of bounds");
                stack.push_back(t->items[idx]);
                break;
            }

            if (auto a = std::get_if<Value::Array>(&object.data)) {
                if (key == "len" || key == "length") {
                    stack.push_back(Value(static_cast<std::int64_t>(a->size())));
                    break;
                }
            }

            if (auto s = std::get_if<std::string>(&object.data)) {
                if (key == "len" || key == "length") {
                    stack.push_back(Value(static_cast<std::int64_t>(s->size())));
                    break;
                }
            }

            auto o = std::get_if<Value::Object>(&object.data);
            if (!o) throw CompileError("T3019: member access requires object");
            auto it = o->find(key);
            if (it == o->end()) throw CompileError("T3020: unknown object member '" + key + "'");
            stack.push_back(it->second);
            break;
        }

        case Op::SetMember: {
            auto value = pop();
            auto object = pop();
            const auto& key = c.names[static_cast<std::size_t>(ins.operand)];
            auto o = std::get_if<Value::Object>(&object.data);
            if (!o) throw CompileError("T3021: member assignment requires object");
            (*o)[key] = std::move(value);
            stack.push_back(std::move(object));
            break;
        }

        case Op::CallMethod: {
            const auto& method_name = c.names[static_cast<std::size_t>(ins.operand)];

            if (method_name == "append") {
                auto arg = pop();
                auto obj = pop();
                if (auto arr = std::get_if<Value::Array>(&obj.data)) {
                    arr->push_back(arg);
                }
                stack.push_back(Value());
                break;
            }
            if (method_name == "pop") {
                auto obj = pop();
                if (auto arr = std::get_if<Value::Array>(&obj.data)) {
                    if (!arr->empty()) {
                        auto val = arr->back();
                        arr->pop_back();
                        stack.push_back(val);
                    } else {
                        stack.push_back(Value());
                    }
                } else {
                    stack.push_back(Value());
                }
                break;
            }
            if (method_name == "len" || method_name == "length") {
                auto obj = pop();
                if (auto arr = std::get_if<Value::Array>(&obj.data)) {
                    stack.push_back(Value(static_cast<std::int64_t>(arr->size())));
                } else if (auto s = std::get_if<std::string>(&obj.data)) {
                    stack.push_back(Value(static_cast<std::int64_t>(s->size())));
                } else {
                    stack.push_back(Value(static_cast<std::int64_t>(0)));
                }
                break;
            }

            // Dynamic Virtual Method Dispatch on Class Instance
            std::string resolved_fn;
            int resolved_idx = -1;

            for (std::size_t s_i = stack.size(); s_i-- > 0;) {
                if (auto o = std::get_if<Value::Object>(&stack[s_i].data)) {
                    auto c_it = o->find("__class__");
                    if (c_it != o->end()) {
                        std::string target_cls = c_it->second.str();
                        std::string candidate = target_cls + "." + method_name;
                        for (std::size_t f_idx = 0; f_idx < c.functions.size(); ++f_idx) {
                            if (c.functions[f_idx].name == candidate) {
                                resolved_fn = candidate;
                                resolved_idx = static_cast<int>(f_idx);
                                break;
                            }
                        }
                        if (resolved_idx >= 0) break;
                    }
                }
            }

            if (resolved_idx < 0) {
                for (std::size_t f_idx = 0; f_idx < c.functions.size(); ++f_idx) {
                    if (c.functions[f_idx].name.find("." + method_name) != std::string::npos ||
                        c.functions[f_idx].name == method_name) {
                        resolved_fn = c.functions[f_idx].name;
                        resolved_idx = static_cast<int>(f_idx);
                        break;
                    }
                }
            }

            if (resolved_idx >= 0) {
                const auto& fn = c.functions[static_cast<std::size_t>(resolved_idx)];
                std::vector<Value> args(fn.params.size());
                for (std::size_t j = fn.params.size(); j-- > 0;) args[j] = pop();
                auto obj = pop();
                frames.push_back({ip, {}});
                auto& frame = frames.back();
                frame.locals["this"] = obj;
                if (auto o = std::get_if<Value::Object>(&obj.data)) {
                    for (const auto& [k, v] : *o) frame.locals[k] = v;
                }
                for (std::size_t j = 0; j < fn.params.size(); ++j)
                    frame.locals[fn.params[j]] = std::move(args[j]);
                ip = static_cast<std::size_t>(fn.entry);
            } else {
                stack.push_back(Value());
            }
            break;
        }

        case Op::Neg: {
            auto v = pop();
            if (auto p = std::get_if<std::int64_t>(&v.data)) stack.emplace_back(-*p);
            else stack.emplace_back(-num(v));
            break;
        }

        case Op::Not:
            stack.emplace_back(!pop().truthy());
            break;

        case Op::BitAnd: {
            auto b = pop(), a = pop();
            stack.emplace_back(std::get<std::int64_t>(a.data) & std::get<std::int64_t>(b.data));
            break;
        }

        case Op::BitOr: {
            auto b = pop(), a = pop();
            stack.emplace_back(std::get<std::int64_t>(a.data) | std::get<std::int64_t>(b.data));
            break;
        }

        case Op::BitXor: {
            auto b = pop(), a = pop();
            stack.emplace_back(std::get<std::int64_t>(a.data) ^ std::get<std::int64_t>(b.data));
            break;
        }

        case Op::ShiftLeft: {
            auto b = pop(), a = pop();
            stack.emplace_back(std::get<std::int64_t>(a.data) << std::get<std::int64_t>(b.data));
            break;
        }

        case Op::ShiftRight: {
            auto b = pop(), a = pop();
            stack.emplace_back(std::get<std::int64_t>(a.data) >> std::get<std::int64_t>(b.data));
            break;
        }

        case Op::Add: {
            auto b = pop(), a = pop();
            if (std::holds_alternative<std::string>(a.data) || std::holds_alternative<std::string>(b.data)) {
                stack.emplace_back(a.str() + b.str());
            } else if (std::holds_alternative<std::int64_t>(a.data) && std::holds_alternative<std::int64_t>(b.data)) {
                stack.emplace_back(std::get<std::int64_t>(a.data) + std::get<std::int64_t>(b.data));
            } else {
                stack.emplace_back(num(a) + num(b));
            }
            break;
        }

        case Op::Sub: {
            auto b = pop(), a = pop();
            if (std::holds_alternative<std::int64_t>(a.data) && std::holds_alternative<std::int64_t>(b.data)) {
                stack.emplace_back(std::get<std::int64_t>(a.data) - std::get<std::int64_t>(b.data));
            } else {
                stack.emplace_back(num(a) - num(b));
            }
            break;
        }

        case Op::Mul: {
            auto b = pop(), a = pop();
            if (std::holds_alternative<std::int64_t>(a.data) && std::holds_alternative<std::int64_t>(b.data)) {
                stack.emplace_back(std::get<std::int64_t>(a.data) * std::get<std::int64_t>(b.data));
            } else {
                stack.emplace_back(num(a) * num(b));
            }
            break;
        }

        case Op::Div: {
            auto b = pop(), a = pop();
            const auto y = num(b);
            if (y == 0.0) throw CompileError("T3010: division by zero");
            if (std::holds_alternative<std::int64_t>(a.data) && std::holds_alternative<std::int64_t>(b.data)) {
                stack.emplace_back(std::get<std::int64_t>(a.data) / std::get<std::int64_t>(b.data));
            } else {
                stack.emplace_back(num(a) / y);
            }
            break;
        }

        case Op::Mod: {
            auto b = pop(), a = pop();
            const auto y = num(b);
            if (y == 0.0) throw CompileError("T3011: modulo by zero");
            if (std::holds_alternative<std::int64_t>(a.data) && std::holds_alternative<std::int64_t>(b.data)) {
                stack.emplace_back(std::get<std::int64_t>(a.data) % std::get<std::int64_t>(b.data));
            } else {
                stack.emplace_back(std::fmod(num(a), y));
            }
            break;
        }

        case Op::Eq: {
            auto b = pop(), a = pop();
            stack.emplace_back(a.str() == b.str());
            break;
        }

        case Op::Ne: {
            auto b = pop(), a = pop();
            stack.emplace_back(a.str() != b.str());
            break;
        }

        case Op::Lt: {
            auto b = pop(), a = pop();
            stack.emplace_back(num(a) < num(b));
            break;
        }

        case Op::Le: {
            auto b = pop(), a = pop();
            stack.emplace_back(num(a) <= num(b));
            break;
        }

        case Op::Gt: {
            auto b = pop(), a = pop();
            stack.emplace_back(num(a) > num(b));
            break;
        }

        case Op::Ge: {
            auto b = pop(), a = pop();
            stack.emplace_back(num(a) >= num(b));
            break;
        }

        case Op::And:
            stack.emplace_back(pop().truthy() && pop().truthy());
            break;

        case Op::Or:
            stack.emplace_back(pop().truthy() || pop().truthy());
            break;

        case Op::Jump:
            ip = static_cast<std::size_t>(ins.operand);
            break;

        case Op::JumpIfFalse: {
            const auto v = pop();
            if (!v.truthy()) ip = static_cast<std::size_t>(ins.operand);
            break;
        }

        case Op::JumpIfTrue: {
            const auto v = pop();
            if (v.truthy()) ip = static_cast<std::size_t>(ins.operand);
            break;
        }

        case Op::PushCatch:
            catch_stack.push_back({static_cast<std::size_t>(ins.operand), stack.size(), frames.size()});
            break;

        case Op::PopCatch:
            if (!catch_stack.empty()) catch_stack.pop_back();
            break;

        case Op::Throw: {
            auto ex_val = pop();
            if (catch_stack.empty()) {
                throw CompileError(ex_val.str());
            }
            auto cf = catch_stack.back();
            catch_stack.pop_back();
            while (stack.size() > cf.stack_size) stack.pop_back();
            while (frames.size() > cf.call_depth) frames.pop_back();
            stack.push_back(ex_val);
            ip = cf.handler_ip;
            break;
        }

        case Op::Print:
            std::cout << pop().str() << '\n';
            break;

        case Op::WebWrite: {
            const auto content = pop().str();
            const auto path = c.constants[static_cast<std::size_t>(ins.operand)].str();
            const auto out = fs::path("dist") / path;
            fs::create_directories(out.parent_path());
            std::ofstream f(out, std::ios::binary);
            if (!f) throw CompileError("T2105: cannot write webfile '" + out.string() + "'");
            f << content;
            break;
        }

        case Op::Call: {
            const auto& fn = c.functions[static_cast<std::size_t>(ins.operand)];
            if (stack.size() < fn.params.size()) throw CompileError("T6012: bytecode stack underflow");
            std::vector<Value> args(fn.params.size());
            for (std::size_t j = fn.params.size(); j-- > 0;) args[j] = pop();
            frames.push_back({ip, {}});
            auto& frame = frames.back();
            for (std::size_t j = 0; j < fn.params.size(); ++j)
                frame.locals[fn.params[j]] = std::move(args[j]);
            ip = static_cast<std::size_t>(fn.entry);
            break;
        }

        case Op::Return: {
            result = pop();
            if (frames.empty()) return result;
            const auto return_ip = frames.back().return_ip;
            frames.pop_back();
            stack.push_back(result);
            ip = return_ip;
            break;
        }
        }
    }

    throw CompileError("T6013: instruction pointer escaped bytecode");
}

} // namespace ternet::bytecode
