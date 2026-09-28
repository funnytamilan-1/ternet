#include "types.hpp"
#include <unordered_map>

namespace ternet::types {
std::string Type::name() const {
    switch(kind) {
    case Kind::Null:return "null";
    case Kind::Bool:return "bool";
    case Kind::Int:return "int";
    case Kind::Float:return "float";
    case Kind::String:return "str";
    case Kind::Array:return "array";
    case Kind::Void:return "void";
    default:return "unknown";
    }
}
namespace {
struct Checker {
    std::vector<std::unordered_map<std::string,Type>> scopes;
    std::unordered_map<std::string,std::vector<Type>> functions;
    Type find(const std::string& n) {
        for(auto it=scopes.rbegin();it!=scopes.rend();++it) {
            auto p=it->find(n); if(p!=it->end()) return p->second;
        }
        throw CheckError("T2001: undefined name '" + n + "'");
    }
    static bool numeric(Type t){return t.kind==Kind::Int||t.kind==Kind::Float;}
    static Type merge_numeric(Type a,Type b){return (a.kind==Kind::Float||b.kind==Kind::Float)?Type{Kind::Float}:Type{Kind::Int};}
    Type expr(const ExprPtr& e) {
        if(!e) throw CheckError("T1002: missing expression");
        switch(e->kind) {
        case Expr::Literal:
            if(std::holds_alternative<std::monostate>(e->literal.data))return {Kind::Null};
            if(std::holds_alternative<bool>(e->literal.data))return {Kind::Bool};
            if(std::holds_alternative<std::int64_t>(e->literal.data))return {Kind::Int};
            if(std::holds_alternative<double>(e->literal.data))return {Kind::Float};
            if(std::holds_alternative<std::string>(e->literal.data))return {Kind::String};
            return {Kind::Unknown};
        case Expr::Variable:return find(e->name);
        case Expr::Array:
            for(auto&a:e->args) expr(a);
            return {Kind::Array};
        case Expr::Unary:{
            auto t=expr(e->right);
            if(e->op=="-"&&!numeric(t))throw CheckError("T3002: unary '-' requires int or float");
            if(e->op=="!"&&t.kind!=Kind::Bool&&t.kind!=Kind::Unknown)throw CheckError("T3002: '!' requires bool");
            return e->op=="!"?Type{Kind::Bool}:t;
        }
        case Expr::Binary:{
            auto a=expr(e->left),b=expr(e->right);
            if(e->op=="+"&&a.kind==Kind::String&&b.kind==Kind::String)return {Kind::String};
            if(e->op=="+"||e->op=="-"||e->op=="*"||e->op=="/"||e->op=="%"){
                if(!numeric(a)||!numeric(b))throw CheckError("T3002: operator '"+e->op+"' requires numeric operands");
                return merge_numeric(a,b);
            }
            if(e->op=="&&"||e->op=="||"){
                if((a.kind!=Kind::Bool&&a.kind!=Kind::Unknown)||(b.kind!=Kind::Bool&&b.kind!=Kind::Unknown))
                    throw CheckError("T3002: logical operator requires bool operands");
                return {Kind::Bool};
            }
            if(e->op=="=="||e->op=="!="||e->op=="<"||e->op=="<="||e->op==">"||e->op==">=")return {Kind::Bool};
            throw CheckError("T3002: unknown operator '"+e->op+"'");
        }
        case Expr::Index:{
            auto a=expr(e->left),i=expr(e->index);
            if(a.kind!=Kind::Array)throw CheckError("T3012: indexing requires an array");
            if(i.kind!=Kind::Int)throw CheckError("T3013: array index requires int");
            return {Kind::Unknown};
        }
        case Expr::Call:{
            if(e->left->kind!=Expr::Variable)throw CheckError("T3014: call target must be a function");
            if(e->left->name=="web_write"){for(auto&a:e->args)expr(a);return {Kind::Void};}
            auto it=functions.find(e->left->name);
            if(it==functions.end())throw CheckError("T2001: undefined function '"+e->left->name+"'");
            if(it->second.size()!=e->args.size())throw CheckError("T3015: wrong argument count for '"+e->left->name+"'");
            for(auto&a:e->args)expr(a);
            return {Kind::Unknown};
        }
        }
        return {Kind::Unknown};
    }
    void body(const std::vector<StmtPtr>& b,bool in_function=false){
        for(const auto&s:b){
            switch(s->kind){
            case Stmt::Let:{auto t=expr(s->expr);if(scopes.back().count(s->name))throw CheckError("T2002: duplicate binding '"+s->name+"'");scopes.back()[s->name]=t;break;}
            case Stmt::Assign:{auto old=find(s->name),now=expr(s->expr);if(old.kind!=Kind::Unknown&&now.kind!=Kind::Unknown&&old.kind!=now.kind)throw CheckError("T3001: cannot assign "+now.name()+" to "+old.name()+" variable '"+s->name+"'");break;}
            case Stmt::Print:for(auto&a:s->print_args)expr(a);break;
            case Stmt::ExprStmt:expr(s->expr);break;
            case Stmt::If:for(auto&x:s->branches){auto t=expr(x.first);if(t.kind!=Kind::Bool&&t.kind!=Kind::Unknown)throw CheckError("T3003: if condition requires bool");scopes.push_back({});body(x.second,in_function);scopes.pop_back();}scopes.push_back({});body(s->else_body,in_function);scopes.pop_back();break;
            case Stmt::While:{auto t=expr(s->expr);if(t.kind!=Kind::Bool&&t.kind!=Kind::Unknown)throw CheckError("T3003: while condition requires bool");scopes.push_back({});body(s->body,in_function);scopes.pop_back();break;}
            case Stmt::Function:{
                functions[s->name]=std::vector<Type>(s->params.size(),Type{Kind::Unknown});
                scopes.push_back({});for(auto&p:s->params)scopes.back()[p]={Kind::Unknown};body(s->function_body,true);scopes.pop_back();break;
            }
            case Stmt::Return:if(!in_function)throw CheckError("T3016: return outside function");if(s->expr)expr(s->expr);break;
            case Stmt::Break:case Stmt::Continue:break;
            case Stmt::Throw:expr(s->expr);break;
            case Stmt::Try:scopes.push_back({});body(s->body,in_function);scopes.pop_back();scopes.push_back({});if(!s->catch_name.empty())scopes.back()[s->catch_name]={Kind::String};body(s->catch_body,in_function);scopes.pop_back();scopes.push_back({});body(s->finally_body,in_function);scopes.pop_back();break;
            case Stmt::WebFile:for(auto&a:s->web_parts)expr(a);break;
            default:break;
            }
        }
    }
    void run(const Program&p){scopes.push_back({});body(p.statements,false);}
};
}
void check(const Program& p){Checker{}.run(p);}
} // namespace ternet::types
