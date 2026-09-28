#include "bytecode.hpp"
#include <fstream>
#include <cmath>
#include <iostream>
#include <limits>
#include <unordered_map>

namespace ternet::bytecode {
namespace {
struct Compiler {
    Chunk c;
    std::vector<std::size_t> break_stack;

    int constant(Value v) {
        c.constants.push_back(std::move(v));
        return static_cast<int>(c.constants.size() - 1);
    }
    int name(const std::string& n) {
        for (std::size_t i = 0; i < c.names.size(); ++i)
            if (c.names[i] == n) return static_cast<int>(i);
        c.names.push_back(n);
        return static_cast<int>(c.names.size() - 1);
    }
    std::size_t emit(Op op, int operand = 0) {
        c.code.push_back({op, operand});
        return c.code.size() - 1;
    }
    void patch(std::size_t at, std::size_t target) {
        if (target > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
            throw CompileError("bytecode too large");
        c.code[at].operand = static_cast<std::int32_t>(target);
    }

    void expr(const ExprPtr& e) {
        if (!e) throw CompileError("T2000: null expression");
        switch (e->kind) {
        case Expr::Literal: emit(Op::Const, constant(e->literal)); return;
        case Expr::Variable: emit(Op::Load, name(e->name)); return;
        case Expr::Unary:
            expr(e->right);
            if (e->op == "-") emit(Op::Neg);
            else if (e->op == "!") emit(Op::Not);
            else throw CompileError("T3002: unsupported unary operator '" + e->op + "'");
            return;
        case Expr::Binary:
            expr(e->left); expr(e->right);
            if (e->op == "+") emit(Op::Add);
            else if (e->op == "-") emit(Op::Sub);
            else if (e->op == "*") emit(Op::Mul);
            else if (e->op == "/") emit(Op::Div);
            else if (e->op == "%") emit(Op::Mod);
            else if (e->op == "==") emit(Op::Eq);
            else if (e->op == "!=") emit(Op::Ne);
            else if (e->op == "<") emit(Op::Lt);
            else if (e->op == "<=") emit(Op::Le);
            else if (e->op == ">") emit(Op::Gt);
            else if (e->op == ">=") emit(Op::Ge);
            else if (e->op == "&&") emit(Op::And);
            else if (e->op == "||") emit(Op::Or);
            else throw CompileError("T3002: unsupported binary operator '" + e->op + "'");
            return;
        default:
            throw CompileError("T2100: expression is not supported by the bytecode compiler yet");
        }
    }

    void statements(const std::vector<StmtPtr>& body) {
        for (const auto& s : body) stmt(s);
    }

    void stmt(const StmtPtr& s) {
        if (!s) throw CompileError("T2000: null statement");
        switch (s->kind) {
        case Stmt::Let:
            expr(s->expr); emit(Op::Store, name(s->name)); return;
        case Stmt::Assign:
            expr(s->expr); emit(Op::Store, name(s->name)); return;
        case Stmt::Print:
            for (const auto& a : s->print_args) { expr(a); emit(Op::Print); }
            return;
        case Stmt::ExprStmt:
            expr(s->expr); emit(Op::Pop); return;
        case Stmt::Return:
            if (s->expr) expr(s->expr); else emit(Op::Const, constant(Value{}));
            emit(Op::Return); return;
        case Stmt::If: {
            std::vector<std::size_t> exits;
            for (const auto& branch : s->branches) {
                expr(branch.first);
                auto jf = emit(Op::JumpIfFalse);
                statements(branch.second);
                auto jend = emit(Op::Jump);
                exits.push_back(jend);
                patch(jf, c.code.size());
            }
            statements(s->else_body);
            for (auto j : exits) patch(j, c.code.size());
            return;
        }
        case Stmt::While: {
            auto start = c.code.size();
            expr(s->expr);
            auto jf = emit(Op::JumpIfFalse);
            statements(s->body);
            emit(Op::Jump, static_cast<int>(start));
            patch(jf, c.code.size());
            return;
        }
        case Stmt::Break:
        case Stmt::Continue:
            throw CompileError("T2101: break/continue require loop-aware lowering and are not yet enabled");
        case Stmt::Function:
            throw CompileError("T2102: function bytecode lowering is not yet enabled");
        case Stmt::Throw:
        case Stmt::Try:
            throw CompileError("T2103: exception bytecode lowering is not yet enabled");
        case Stmt::WebFile:
            throw CompileError("T2104: webfile bytecode lowering is not yet enabled");
        default:
            throw CompileError("T2100: statement is not supported by the bytecode compiler yet");
        }
    }
};
}

Chunk compile(const Program& program) {
    Compiler x;
    x.statements(program.statements);
    x.emit(Op::Halt);
    verify(x.c);
    return x.c;
}

void verify(const Chunk& c) {
    if (c.code.empty()) throw CompileError("T6001: empty bytecode");
    for (std::size_t i = 0; i < c.code.size(); ++i) {
        const auto& ins = c.code[i];
        auto valid_index = [](int n, std::size_t size) { return n >= 0 && static_cast<std::size_t>(n) < size; };
        switch (ins.op) {
        case Op::Const:
            if (!valid_index(ins.operand, c.constants.size())) throw CompileError("T6002: invalid constant index");
            break;
        case Op::Load:
        case Op::Store:
            if (!valid_index(ins.operand, c.names.size())) throw CompileError("T6003: invalid name index");
            break;
        case Op::Jump:
        case Op::JumpIfFalse:
            if (!valid_index(ins.operand, c.code.size())) throw CompileError("T6004: invalid jump target");
            break;
        default: break;
        }
    }
}

void write(const Chunk& c, const std::string& path) {
    verify(c);
    std::ofstream f(path, std::ios::binary);
    if (!f) throw CompileError("T6005: cannot open bytecode output '" + path + "'");
    const std::uint32_t magic = 0x544E4231; // TNB1
    const std::uint32_t nc = static_cast<std::uint32_t>(c.constants.size());
    const std::uint32_t nn = static_cast<std::uint32_t>(c.names.size());
    const std::uint32_t ni = static_cast<std::uint32_t>(c.code.size());
    f.write(reinterpret_cast<const char*>(&magic), sizeof magic);
    f.write(reinterpret_cast<const char*>(&nc), sizeof nc);
    f.write(reinterpret_cast<const char*>(&nn), sizeof nn);
    f.write(reinterpret_cast<const char*>(&ni), sizeof ni);
    for (const auto& v : c.constants) {
        const auto s = v.str();
        const std::uint32_t n = static_cast<std::uint32_t>(s.size());
        f.write(reinterpret_cast<const char*>(&n), sizeof n);
        f.write(s.data(), static_cast<std::streamsize>(s.size()));
    }
    for (const auto& n : c.names) {
        const std::uint32_t len = static_cast<std::uint32_t>(n.size());
        f.write(reinterpret_cast<const char*>(&len), sizeof len);
        f.write(n.data(), static_cast<std::streamsize>(n.size()));
    }
    for (const auto& ins : c.code) {
        const auto op = static_cast<std::uint8_t>(ins.op);
        f.write(reinterpret_cast<const char*>(&op), sizeof op);
        f.write(reinterpret_cast<const char*>(&ins.operand), sizeof ins.operand);
    }
    if (!f) throw CompileError("T6006: failed writing bytecode");
}

Chunk read(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw CompileError("T6007: cannot open bytecode '" + path + "'");
    std::uint32_t magic=0,nc=0,nn=0,ni=0;
    f.read(reinterpret_cast<char*>(&magic),4);
    f.read(reinterpret_cast<char*>(&nc),4);
    f.read(reinterpret_cast<char*>(&nn),4);
    f.read(reinterpret_cast<char*>(&ni),4);
    if (magic != 0x544E4231) throw CompileError("T6008: invalid Ternet bytecode");
    if (nc > 1000000 || nn > 1000000 || ni > 10000000) throw CompileError("T6009: unreasonable bytecode size");
    Chunk c;
    // Serialized constants currently use their display form. This reader accepts
    // strings as constants; production typed constant encoding will replace this
    // compatibility format before bytecode becomes a stable public ABI.
    for (std::uint32_t i=0;i<nc;++i) {
        std::uint32_t n=0; f.read(reinterpret_cast<char*>(&n),4);
        std::string s(n,'\0'); f.read(s.data(), static_cast<std::streamsize>(n));
        c.constants.emplace_back(s);
    }
    for (std::uint32_t i=0;i<nn;++i) {
        std::uint32_t n=0; f.read(reinterpret_cast<char*>(&n),4);
        std::string s(n,'\0'); f.read(s.data(), static_cast<std::streamsize>(n));
        c.names.push_back(std::move(s));
    }
    for (std::uint32_t i=0;i<ni;++i) {
        std::uint8_t op=0; std::int32_t operand=0;
        f.read(reinterpret_cast<char*>(&op),1); f.read(reinterpret_cast<char*>(&operand),4);
        if (!f || op > static_cast<std::uint8_t>(Op::Return)) throw CompileError("T6010: invalid opcode");
        c.code.push_back({static_cast<Op>(op),operand});
    }
    if (!f) throw CompileError("T6011: truncated bytecode");
    verify(c);
    return c;
}

Value execute(const Chunk& c) {
    verify(c);
    std::vector<Value> stack;
    std::unordered_map<std::string,Value> vars;
    auto pop=[&](){ if(stack.empty()) throw CompileError("T6012: bytecode stack underflow"); auto v=stack.back(); stack.pop_back(); return v; };
    auto num=[](const Value& v)->double {
        if(auto p=std::get_if<std::int64_t>(&v.data)) return static_cast<double>(*p);
        if(auto p=std::get_if<double>(&v.data)) return *p;
        throw CompileError("T3002: numeric operator requires numbers");
    };
    Value result;
    std::size_t ip=0;
    while(ip<c.code.size()) {
        const auto ins=c.code[ip++];
        switch(ins.op) {
        case Op::Halt: return result;
        case Op::Const: stack.push_back(c.constants[ins.operand]); break;
        case Op::Load: {
            auto it=vars.find(c.names[ins.operand]);
            if(it==vars.end()) throw CompileError("T2001: undefined variable '" + c.names[ins.operand] + "'");
            stack.push_back(it->second); break;
        }
        case Op::Store: vars[c.names[ins.operand]]=pop(); break;
        case Op::Pop: pop(); break;
        case Op::Neg: { auto v=pop(); if(auto p=std::get_if<std::int64_t>(&v.data)) stack.emplace_back(-*p); else stack.emplace_back(-num(v)); break; }
        case Op::Not: stack.emplace_back(!pop().truthy()); break;
        case Op::Add: {auto b=pop(),a=pop(); if(std::holds_alternative<std::string>(a.data)||std::holds_alternative<std::string>(b.data)) stack.emplace_back(a.str()+b.str()); else stack.emplace_back(num(a)+num(b)); break;}
        case Op::Sub: {auto b=pop(),a=pop(); stack.emplace_back(num(a)-num(b)); break;}
        case Op::Mul: {auto b=pop(),a=pop(); stack.emplace_back(num(a)*num(b)); break;}
        case Op::Div: {auto b=pop(),a=pop(); auto y=num(b); if(y==0) throw CompileError("T3010: division by zero"); stack.emplace_back(num(a)/y); break;}
        case Op::Mod: {auto b=pop(),a=pop(); auto y=num(b); if(y==0) throw CompileError("T3011: modulo by zero"); stack.emplace_back(std::fmod(num(a),y)); break;}
        case Op::Eq: {auto b=pop(),a=pop(); stack.emplace_back(a.str()==b.str()); break;}
        case Op::Ne: {auto b=pop(),a=pop(); stack.emplace_back(a.str()!=b.str()); break;}
        case Op::Lt: {auto b=pop(),a=pop(); stack.emplace_back(num(a)<num(b)); break;}
        case Op::Le: {auto b=pop(),a=pop(); stack.emplace_back(num(a)<=num(b)); break;}
        case Op::Gt: {auto b=pop(),a=pop(); stack.emplace_back(num(a)>num(b)); break;}
        case Op::Ge: {auto b=pop(),a=pop(); stack.emplace_back(num(a)>=num(b)); break;}
        case Op::And: {auto b=pop(),a=pop(); stack.emplace_back(a.truthy()&&b.truthy()); break;}
        case Op::Or: {auto b=pop(),a=pop(); stack.emplace_back(a.truthy()||b.truthy()); break;}
        case Op::Jump: ip=static_cast<std::size_t>(ins.operand); break;
        case Op::JumpIfFalse: if(!pop().truthy()) ip=static_cast<std::size_t>(ins.operand); break;
        case Op::Print: std::cout << pop().str() << '\n'; break;
        case Op::Return: result=pop(); return result;
        }
    }
    throw CompileError("T6013: instruction pointer escaped bytecode");
}
} // namespace ternet::bytecode
