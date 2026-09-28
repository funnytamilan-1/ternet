#include "ternet.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace ternet{
bool Value::truthy()const{if(std::holds_alternative<std::monostate>(data))return false;if(auto p=std::get_if<bool>(&data))return *p;if(auto p=std::get_if<std::int64_t>(&data))return *p!=0;if(auto p=std::get_if<double>(&data))return *p!=0;if(auto p=std::get_if<std::string>(&data))return !p->empty();if(auto p=std::get_if<Array>(&data))return !p->empty();return true;}
std::string Value::str()const{if(std::holds_alternative<std::monostate>(data))return"null";if(auto p=std::get_if<bool>(&data))return*p?"true":"false";if(auto p=std::get_if<std::int64_t>(&data))return std::to_string(*p);if(auto p=std::get_if<double>(&data)){auto s=std::to_string(*p);while(s.size()>1&&s.back()=='0')s.pop_back();if(s.back()=='.')s.pop_back();return s;}if(auto p=std::get_if<std::string>(&data))return*p;if(auto p=std::get_if<Array>(&data)){std::string s="[";for(size_t i=0;i<p->size();++i){if(i)s+=", ";s+=(*p)[i].str();}return s+"]";}return"<object>";}
Interpreter::Binding*Interpreter::find(const std::string&n){for(auto i=scopes.rbegin();i!=scopes.rend();++i){auto q=i->find(n);if(q!=i->end())return&q->second;}return nullptr;}
Value Interpreter::binary(const std::string&o,const Value&a,const Value&b){if(o=="&&")return a.truthy()&&b.truthy();if(o=="||")return a.truthy()||b.truthy();if(o=="==")return a.str()==b.str();if(o=="!=")return a.str()!=b.str();auto num=[](const Value&v){if(auto p=std::get_if<std::int64_t>(&v.data))return(double)*p;if(auto p=std::get_if<double>(&v.data))return*p;throw RuntimeError("numeric operator requires numbers");};if(o=="+"){if(std::holds_alternative<std::string>(a.data)||std::holds_alternative<std::string>(b.data))return a.str()+b.str();double z=num(a)+num(b);if(std::holds_alternative<std::int64_t>(a.data)&&std::holds_alternative<std::int64_t>(b.data))return Value((std::int64_t)z);return Value(z);}double x=num(a),y=num(b);if(o=="-")return x-y;if(o=="*")return x*y;if(o=="/"){if(y==0)throw RuntimeError("division by zero");return x/y;}if(o=="%")return std::fmod(x,y);if(o==">")return x>y;if(o=="<")return x<y;if(o==">=")return x>=y;if(o=="<=")return x<=y;throw RuntimeError("unknown operator "+o);}
Value Interpreter::eval(const ExprPtr&e){if(!e)throw RuntimeError("internal error: null expression");switch(e->kind){case Expr::Literal:return e->literal;case Expr::Variable:{auto b=find(e->name);if(!b)throw RuntimeError("undefined variable '"+e->name+"'");return b->value;}case Expr::Unary:{auto v=eval(e->right);if(e->op=="!")return!v.truthy();if(e->op=="-"){if(auto p=std::get_if<std::int64_t>(&v.data))return-*p;if(auto p=std::get_if<double>(&v.data))return-*p;}throw RuntimeError("invalid unary operator");}case Expr::Binary:return binary(e->op,eval(e->left),eval(e->right));case Expr::Array:{Value::Array a;for(auto&q:e->args)a.push_back(eval(q));return a;}case Expr::Index:{auto v=eval(e->left),idx=eval(e->index);auto a=std::get_if<Value::Array>(&v.data);auto n=std::get_if<std::int64_t>(&idx.data);if(!a||!n||*n<0||(size_t)*n>=a->size())throw RuntimeError("array index out of bounds");return(*a)[(size_t)*n];}case Expr::Call:{if(e->left->kind!=Expr::Variable)throw RuntimeError("call target must be a function");if(e->left->name=="web_write"){if(e->args.size()!=2)throw RuntimeError("web_write(path, content) expects 2 arguments");auto path=eval(e->args[0]).str();auto content=eval(e->args[1]).str();std::filesystem::path out=std::filesystem::path("dist")/path;if(out.string().find("..")!=std::string::npos)throw RuntimeError("web_write path may not escape dist");std::filesystem::create_directories(out.parent_path());std::ofstream f(out,std::ios::binary);if(!f)throw RuntimeError("cannot write web file '"+out.string()+"'");f<<content;return Value{};}auto it=functions.find(e->left->name);if(it==functions.end())throw RuntimeError("undefined function '"+e->left->name+"'");if(it->second.params.size()!=e->args.size())throw RuntimeError("wrong argument count for '"+e->left->name+"'");scopes.push_back({});for(size_t i=0;i<e->args.size();++i)scopes.back()[it->second.params[i]]={eval(e->args[i]),true};returning=false;return_value={};exec_all(it->second.body);auto r=return_value;returning=false;scopes.pop_back();return r;}}throw RuntimeError("invalid expression");}
void Interpreter::exec(const StmtPtr&s){
if(returning||breaking||continuing)return;
if(s->kind==Stmt::Throw){throw RuntimeError(eval(s->expr).str());}
if(s->kind==Stmt::Try){
    try{exec_all(s->body);}
    catch(const std::exception&e){
        if(!s->catch_body.empty()){
            scopes.push_back({});
            if(!s->catch_name.empty())scopes.back()[s->catch_name]={std::string(e.what()),true};
            exec_all(s->catch_body);
            scopes.pop_back();
        } else {
            if(!s->finally_body.empty())exec_all(s->finally_body);
            throw;
        }
    }
    if(!s->finally_body.empty())exec_all(s->finally_body);
    return;
}
if(s->kind==Stmt::WebFile){
    std::filesystem::path relative=s->web_path;
    if(relative.is_absolute()||s->web_path.empty()||s->web_path.find("..")!=std::string::npos)throw RuntimeError("webfile path must be a non-empty relative path inside dist");
    auto out=std::filesystem::path("dist")/relative;
    std::filesystem::create_directories(out.parent_path());
    std::ofstream f(out,std::ios::binary);
    if(!f)throw RuntimeError("cannot write web file '"+out.string()+"'");
    for(const auto&part:s->web_parts)f<<eval(part).str();
    return;
}
switch(s->kind){case Stmt::Let:if(scopes.back().count(s->name))throw RuntimeError("binding already exists: "+s->name);scopes.back()[s->name]={eval(s->expr),s->mutable_binding};break;case Stmt::Assign:{auto b=find(s->name);if(!b)throw RuntimeError("undefined variable '"+s->name+"'");if(!b->mutable_binding)throw RuntimeError("cannot assign to immutable binding '"+s->name+"'");b->value=eval(s->expr);break;}case Stmt::Print:{for(size_t i=0;i<s->print_args.size();++i){if(i)std::cout<<' ';std::cout<<eval(s->print_args[i]).str();}std::cout<<'\n';break;}case Stmt::ExprStmt:eval(s->expr);break;case Stmt::Function:functions[s->name]={s->params,s->function_body};break;case Stmt::Return:return_value=s->expr?eval(s->expr):Value{};returning=true;break;case Stmt::Break:breaking=true;break;case Stmt::Continue:continuing=true;break;case Stmt::If:{for(auto&b:s->branches)if(eval(b.first).truthy()){exec_all(b.second);return;}exec_all(s->else_body);break;}case Stmt::While:while(eval(s->expr).truthy()){exec_all(s->body);if(returning)break;if(breaking){breaking=false;break;}if(continuing){continuing=false;continue;}}break;case Stmt::For:{auto a=eval(s->for_start),b=eval(s->for_end);auto x=std::get_if<std::int64_t>(&a.data),y=std::get_if<std::int64_t>(&b.data);if(!x||!y)throw RuntimeError("for range requires int bounds");scopes.push_back({});scopes.back()[s->name]={Value(*x),true};for(std::int64_t n=*x;n<=*y;++n){scopes.back()[s->name].value=Value(n);exec_all(s->body);if(returning)break;if(breaking){breaking=false;break;}if(continuing){continuing=false;}}scopes.pop_back();break;}default:throw RuntimeError("unsupported statement");}}
void Interpreter::exec_all(const std::vector<StmtPtr>&b){for(auto&s:b){exec(s);if(returning||breaking||continuing)break;}}
void Interpreter::run(const Program&p){scopes.clear();functions.clear();returning=breaking=continuing=false;scopes.push_back({});exec_all(p.statements);}
}