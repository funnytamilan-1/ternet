#include "ternet.hpp"
namespace ternet{
class Parser{const std::vector<Token>&t;size_t i=0;Token&c(){return const_cast<Token&>(t[i]);}
bool is(TokenType k,const char*x=nullptr){return c().type==k&&(!x||c().text==x);}bool eat(TokenType k,const char*x=nullptr){if(is(k,x)){++i;return true;}return false;}
void need(TokenType k,const char*m){if(!eat(k))throw RuntimeError(std::string(m)+" at "+std::to_string(c().pos.line)+":"+std::to_string(c().pos.column));}
ExprPtr expr(){return eq();}ExprPtr eq(){auto x=cmp();while(is(TokenType::Op,"==")||is(TokenType::Op,"!=")){auto o=c().text;++i;x=bin(o,x,cmp());}return x;}
ExprPtr cmp(){auto x=term();while(is(TokenType::Op,">")||is(TokenType::Op,"<")||is(TokenType::Op,">=")||is(TokenType::Op,"<=")){auto o=c().text;++i;x=bin(o,x,term());}return x;}
ExprPtr term(){auto x=factor();while(is(TokenType::Op,"+")||is(TokenType::Op,"-")||is(TokenType::Op,"||")){auto o=c().text;++i;x=bin(o,x,factor());}return x;}
ExprPtr factor(){auto x=unary();while(is(TokenType::Op,"*")||is(TokenType::Op,"/")||is(TokenType::Op,"%")||is(TokenType::Op,"&&")){auto o=c().text;++i;x=bin(o,x,unary());}return x;}
ExprPtr bin(const std::string&o,ExprPtr a,ExprPtr b){auto n=std::make_shared<Expr>();n->kind=Expr::Binary;n->op=o;n->left=a;n->right=b;return n;}
ExprPtr unary(){if(is(TokenType::Op,"!")||is(TokenType::Op,"-")){auto n=std::make_shared<Expr>();n->kind=Expr::Unary;n->op=c().text;++i;n->right=unary();return n;}return post();}
ExprPtr post(){auto x=primary();for(;;){if(eat(TokenType::LParen)){auto n=std::make_shared<Expr>();n->kind=Expr::Call;n->left=x;if(!eat(TokenType::RParen)){do{n->args.push_back(expr());}while(eat(TokenType::Comma));need(TokenType::RParen,"expected ')'");}x=n;}else if(eat(TokenType::LBracket)){auto n=std::make_shared<Expr>();n->kind=Expr::Index;n->left=x;n->index=expr();need(TokenType::RBracket,"expected ']'");x=n;}else if(eat(TokenType::Dot)){if(!is(TokenType::Identifier))throw RuntimeError("expected member name");auto n=std::make_shared<Expr>();n->kind=Expr::Member;n->object=x;n->name=c().text;++i;x=n;}else break;}return x;}
ExprPtr primary(){if(is(TokenType::Number)){auto n=std::make_shared<Expr>();n->kind=Expr::Literal;auto s=c().text;n->literal=s.find('.')==std::string::npos?Value((std::int64_t)std::stoll(s)):Value(std::stod(s));++i;return n;}
if(is(TokenType::String)){auto n=std::make_shared<Expr>();n->kind=Expr::Literal;n->literal=c().text;++i;return n;}
if(is(TokenType::Keyword,"true")||is(TokenType::Keyword,"false")){auto n=std::make_shared<Expr>();n->kind=Expr::Literal;n->literal=(c().text=="true");++i;return n;}
if(is(TokenType::Keyword,"null")){auto n=std::make_shared<Expr>();n->kind=Expr::Literal;++i;return n;}
if(is(TokenType::Identifier)||is(TokenType::Keyword,"tnprint")){auto n=std::make_shared<Expr>();n->kind=Expr::Variable;n->name=c().text;++i;return n;}
if(eat(TokenType::LParen)){auto n=expr();need(TokenType::RParen,"expected ')'");return n;}
if(eat(TokenType::LBracket)){auto n=std::make_shared<Expr>();n->kind=Expr::Array;if(!eat(TokenType::RBracket)){do{n->args.push_back(expr());}while(eat(TokenType::Comma));need(TokenType::RBracket,"expected ']'");}return n;}
throw RuntimeError("expected expression at "+std::to_string(c().pos.line)+":"+std::to_string(c().pos.column));}
void end(){if(!eat(TokenType::Colon)&&!eat(TokenType::Semicolon))throw RuntimeError("expected ':' at line "+std::to_string(c().pos.line));}
std::vector<StmtPtr>braced(){std::vector<StmtPtr>b;need(TokenType::LBrace,"expected '{'");while(!is(TokenType::RBrace)&&!is(TokenType::End))b.push_back(stmt());need(TokenType::RBrace,"expected '}'");return b;}
std::string type(){if(is(TokenType::Keyword)||is(TokenType::Identifier)){auto x=c().text;++i;return x;}throw RuntimeError("expected type at "+std::to_string(c().pos.line));}
StmtPtr declaration(bool explicit_type=false){auto s=std::make_shared<Stmt>();s->kind=Stmt::Let;s->mutable_binding=true;if(explicit_type)s->type_name=type();if(!is(TokenType::Identifier))throw RuntimeError("expected binding name");s->name=c().text;++i;need(TokenType::Op,"expected '='");s->expr=expr();end();return s;}
StmtPtr stmt(){auto s=std::make_shared<Stmt>();s->pos=c().pos;
if(is(TokenType::Keyword,"webfile")){++i;if(!is(TokenType::String))throw RuntimeError("expected output path after webfile");s->kind=Stmt::WebFile;s->web_path=c().text;++i;need(TokenType::LBrace,"expected '{' after webfile path");while(!is(TokenType::RBrace)&&!is(TokenType::End)){s->web_parts.push_back(expr());end();}need(TokenType::RBrace,"expected '}' after webfile");eat(TokenType::Colon);return s;}
if(is(TokenType::Keyword,"let")||is(TokenType::Keyword,"mut")||is(TokenType::Keyword,"const")){auto k=c().text;++i;s=declaration();s->mutable_binding=k=="mut";return s;}
if(is(TokenType::Keyword,"int")||is(TokenType::Keyword,"str")||is(TokenType::Keyword,"float")||is(TokenType::Keyword,"bool")||is(TokenType::Keyword,"size"))return declaration(true);
if(is(TokenType::Keyword,"tnprint")){++i;need(TokenType::LParen,"expected '(' after tnprint");s->kind=Stmt::Print;s->print_args.push_back(expr());while(eat(TokenType::Comma))s->print_args.push_back(expr());need(TokenType::RParen,"expected ')'");end();return s;}
if(is(TokenType::Keyword,"if")){s->kind=Stmt::If;++i;need(TokenType::LBrace,"expected '{' after if");auto cond=expr();need(TokenType::RBrace,"expected '}'");eat(TokenType::Semicolon);s->branches.push_back({cond,after_header()});while(eat(TokenType::Keyword,"elif")){need(TokenType::LBrace,"expected '{' after elif");auto cc=expr();need(TokenType::RBrace,"expected '}'");eat(TokenType::Semicolon);s->branches.push_back({cc,after_header()});}if(eat(TokenType::Keyword,"else")){if(eat(TokenType::LBrace)){if(eat(TokenType::RBrace)){eat(TokenType::Semicolon);s->else_body.push_back(stmt());}else{while(!is(TokenType::RBrace)&&!is(TokenType::End))s->else_body.push_back(stmt());need(TokenType::RBrace,"expected '}'");eat(TokenType::Semicolon);}}else s->else_body.push_back(stmt());}return s;}
if(is(TokenType::Keyword,"while")){s->kind=Stmt::While;++i;need(TokenType::LBrace,"expected '{' after while");s->expr=expr();need(TokenType::RBrace,"expected '}'");eat(TokenType::Semicolon);s->body=is(TokenType::LBrace)?braced():std::vector<StmtPtr>{stmt()};return s;}
if(is(TokenType::Keyword,"for")){s->kind=Stmt::For;++i;if(!is(TokenType::Identifier))throw RuntimeError("expected loop variable");s->name=c().text;++i;need(TokenType::Keyword,"expected 'in'");if(t[i-1].text!="in")throw RuntimeError("expected 'in'");s->for_start=expr();if(eat(TokenType::Op,"..")){s->for_end=expr();s->for_inclusive=true;}else throw RuntimeError("for requires range '..'");s->body=braced();return s;}
if(is(TokenType::Keyword,"enum")){
++i; if(!is(TokenType::Identifier))throw RuntimeError("expected enum name");
s->kind=Stmt::Enum; s->name=c().text; ++i; need(TokenType::LBrace,"expected '{' after enum name");
while(!is(TokenType::RBrace)&&!is(TokenType::End)){ if(!is(TokenType::Identifier))throw RuntimeError("expected enum value"); s->enum_values.push_back(c().text); ++i; if(!eat(TokenType::Comma)) eat(TokenType::Colon); }
need(TokenType::RBrace,"expected '}' after enum"); eat(TokenType::Colon); return s;}
if(is(TokenType::Keyword,"match")){
++i; s->kind=Stmt::Match; s->match_expr=expr(); need(TokenType::LBrace,"expected '{' after match expression");
while(!is(TokenType::RBrace)&&!is(TokenType::End)){
  if(is(TokenType::Identifier,"_")){ ++i; need(TokenType::Op,"expected '=>'"); if(t[i-1].text!="=>")throw RuntimeError("expected '=>'"); s->match_default=after_header(); }
  else { auto pattern=expr(); need(TokenType::Op,"expected '=>'"); if(t[i-1].text!="=>")throw RuntimeError("expected '=>'"); s->match_cases.push_back({pattern,after_header()}); }
}
need(TokenType::RBrace,"expected '}' after match"); eat(TokenType::Colon); return s;}
if(is(TokenType::Keyword,"struct")||is(TokenType::Keyword,"class")){++i;if(!is(TokenType::Identifier))throw RuntimeError("expected struct name");s->kind=Stmt::Struct;s->name=c().text;++i;need(TokenType::LBrace,"expected '{' after struct name");while(!is(TokenType::RBrace)&&!is(TokenType::End)){if(!is(TokenType::Keyword)&&!is(TokenType::Identifier))throw RuntimeError("expected field type");++i;if(!is(TokenType::Identifier))throw RuntimeError("expected field name");s->fields.push_back(c().text);++i;end();}need(TokenType::RBrace,"expected '}' after struct");eat(TokenType::Colon);return s;}
if(is(TokenType::Keyword,"fn")||is(TokenType::Keyword,"lit")){++i;if(!is(TokenType::Identifier))throw RuntimeError("expected function name");s->kind=Stmt::Function;s->name=c().text;++i;need(TokenType::LParen,"expected '('");if(!eat(TokenType::RParen)){do{std::string pt; if(is(TokenType::Keyword)||is(TokenType::Identifier)){auto save=i;auto candidate=c().text;++i;if(is(TokenType::Identifier)){pt=candidate;}else i=save;} if(!is(TokenType::Identifier))throw RuntimeError("expected parameter");s->param_types.push_back(pt);s->params.push_back(c().text);++i;}while(eat(TokenType::Comma));need(TokenType::RParen,"expected ')'");}if(eat(TokenType::Colon)){s->return_type=type();}s->function_body=braced();eat(TokenType::Colon);return s;}
if(is(TokenType::Keyword,"throw")){++i;s->kind=Stmt::Throw;s->expr=expr();end();return s;}
if(is(TokenType::Keyword,"try")){++i;s->kind=Stmt::Try;s->body=braced();if(eat(TokenType::Keyword,"catch")){if(is(TokenType::Identifier)){s->catch_name=c().text;++i;}s->catch_body=braced();}if(eat(TokenType::Keyword,"finally"))s->finally_body=braced();if(s->catch_body.empty()&&s->finally_body.empty())throw RuntimeError("try requires catch or finally at "+std::to_string(s->pos.line)+":"+std::to_string(s->pos.column));return s;}
if(is(TokenType::Keyword,"return")){++i;s->kind=Stmt::Return;if(!is(TokenType::Colon))s->expr=expr();end();return s;}
if(is(TokenType::Keyword,"break")){++i;s->kind=Stmt::Break;end();return s;}if(is(TokenType::Keyword,"continue")){++i;s->kind=Stmt::Continue;end();return s;}
if(is(TokenType::Identifier)){auto save=i;auto lhs=post();if(eat(TokenType::Op,"=")){s->kind=Stmt::Assign;s->target=lhs;s->name=lhs->kind==Expr::Variable?lhs->name:"";s->expr=expr();end();return s;}i=save;}
s->kind=Stmt::ExprStmt;s->expr=expr();end();return s;}
std::vector<StmtPtr>after_header(){if(is(TokenType::LBrace))return braced();return{stmt()};}
public:Parser(const std::vector<Token>&x):t(x){}Program parse(){Program p;while(!is(TokenType::End))p.statements.push_back(stmt());return p;}};
Program parse(const std::vector<Token>&t){return Parser(t).parse();}
}