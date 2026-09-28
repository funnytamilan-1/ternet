#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
namespace ternet {
struct SourcePos{std::size_t line=1,column=1;};
enum class TokenType{Identifier,Number,String,Keyword,Op,LParen,RParen,LBrace,RBrace,LBracket,RBracket,Comma,Colon,Semicolon,Dot,End};
struct Token{TokenType type;std::string text;SourcePos pos;};
std::vector<Token> lex(const std::string&);
struct Value{using Array=std::vector<Value>;using Object=std::unordered_map<std::string,Value>;std::variant<std::monostate,bool,std::int64_t,double,std::string,Array,Object> data;Value()=default;template<class T>Value(T v):data(std::move(v)){}bool truthy()const;std::string str()const;};
struct Expr;using ExprPtr=std::shared_ptr<Expr>;
struct Expr{enum Kind{Literal,Variable,Unary,Binary,Call,Array,Index}kind;Value literal;std::string name,op;ExprPtr left,right;std::vector<ExprPtr>args;ExprPtr index;};
struct Stmt;using StmtPtr=std::shared_ptr<Stmt>;
struct Stmt{enum Kind{ExprStmt,Let,Assign,Print,If,While,For,Function,Return,Break,Continue,Block}kind;SourcePos pos;std::string name;bool mutable_binding=true;ExprPtr expr;std::vector<ExprPtr>print_args;std::vector<StmtPtr>body;std::vector<std::pair<ExprPtr,std::vector<StmtPtr>>>branches;std::vector<StmtPtr>else_body;std::vector<std::string>params;std::vector<StmtPtr>function_body;};
struct Program{std::vector<StmtPtr>statements;};Program parse(const std::vector<Token>&);
class RuntimeError:public std::runtime_error{public:using std::runtime_error::runtime_error;};
class Interpreter{public:void run(const Program&);private:struct Binding{Value value;bool mutable_binding=true;};struct Function{std::vector<std::string>params;std::vector<StmtPtr>body;};std::vector<std::unordered_map<std::string,Binding>>scopes;std::unordered_map<std::string,Function>functions;bool returning=false,breaking=false,continuing=false;Value return_value;Value eval(const ExprPtr&);void exec(const StmtPtr&);void exec_all(const std::vector<StmtPtr>&);Binding*find(const std::string&);static Value binary(const std::string&,const Value&,const Value&);};
int command_init(const std::string&);int command_package(const std::string&);int command_add(const std::string&,const std::string&,const std::string&);int command_install(const std::string&);int command_remove(const std::string&,const std::string&);int command_list(const std::string&);
}