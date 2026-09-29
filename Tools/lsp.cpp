#include "lsp.hpp"
#include "ternet.hpp"
#include "types.hpp"
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ternet::lsp {
namespace {
struct Symbol { std::string name; SourcePos pos; std::string detail; };
struct Message { std::string id, method, params; };

std::string esc(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == '\\') r += "\\\\";
        else if (c == '"') r += "\\\"";
        else if (c == '\n') r += "\\n";
        else if (c == '\r') r += "\\r";
        else if (c == '\t') r += "\\t";
        else r += c;
    }
    return r;
}
std::string q(const std::string& s) { return "\"" + esc(s) + "\""; }
std::string str_field(const std::string& j, const std::string& key) {
    const std::string k = "\"" + key + "\"";
    auto p = j.find(k); if (p == std::string::npos) return {};
    p = j.find(':', p + k.size()); if (p == std::string::npos) return {};
    p = j.find('"', p + 1); if (p == std::string::npos) return {};
    ++p; std::string r; bool slash = false;
    for (; p < j.size(); ++p) {
        char c = j[p];
        if (slash) { if (c == 'n') r += '\n'; else if (c == 'r') r += '\r'; else if (c == 't') r += '\t'; else r += c; slash = false; }
        else if (c == '\\') slash = true;
        else if (c == '"') break;
        else r += c;
    }
    return r;
}
int int_field(const std::string& j, const std::string& key) {
    const std::string k = "\"" + key + "\""; auto p = j.find(k);
    if (p == std::string::npos) return 0; p = j.find(':', p + k.size()); if (p == std::string::npos) return 0; ++p;
    while (p < j.size() && std::isspace((unsigned char)j[p])) ++p; int n = 0;
    while (p < j.size() && std::isdigit((unsigned char)j[p])) n = n * 10 + j[p++] - '0'; return n;
}
std::string field_uri(const std::string& p) { return str_field(p, "uri"); }
std::string position_range(SourcePos p, int width = 1) {
    int l = p.line ? (int)p.line - 1 : 0, c = p.column ? (int)p.column - 1 : 0;
    return "{\"start\":{\"line\":" + std::to_string(l) + ",\"character\":" + std::to_string(c) + "},\"end\":{\"line\":" + std::to_string(l) + ",\"character\":" + std::to_string(c + width) + "}}";
}
std::string word_at(const std::string& s, int line, int col) {
    size_t p = 0, ln = 0; while (p < s.size() && ln < (size_t)line) if (s[p++] == '\n') ++ln;
    if (ln != (size_t)line) return {}; p = std::min(s.size(), p + (size_t)col); size_t a = p, b = p;
    while (a && (std::isalnum((unsigned char)s[a - 1]) || s[a - 1] == '_')) --a;
    while (b < s.size() && (std::isalnum((unsigned char)s[b]) || s[b] == '_')) ++b; return s.substr(a, b - a);
}
void collect(const std::vector<StmtPtr>& xs, std::vector<Symbol>& out) {
    for (const auto& s : xs) if (s) {
        if (s->kind == Stmt::Let || s->kind == Stmt::Function || s->kind == Stmt::Struct || s->kind == Stmt::Enum || s->kind == Stmt::Class || s->kind == Stmt::Trait)
            out.push_back({s->name, s->pos, s->kind == Stmt::Function ? "function" : "declaration"});
        if (s->kind == Stmt::Function) collect(s->function_body, out);
        if (s->kind == Stmt::Class || s->kind == Stmt::Trait) collect(s->methods, out);
    }
}
std::string diag(const RuntimeError& e) {
    return "{\"range\":" + position_range(e.pos) + ",\"severity\":1,\"source\":\"ternet\",\"code\":" + q(e.code) + ",\"message\":" + q(e.what()) + "}";
}

class Server {
    std::unordered_map<std::string, std::string> docs;
    void send(std::ostream& o, const std::string& body) { o << "Content-Length: " << body.size() << "\r\n\r\n" << body << std::flush; }
    void result(std::ostream& o, const std::string& id, const std::string& value) { send(o, "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "null" : id) + ",\"result\":" + value + "}"); }
    void error(std::ostream& o, const std::string& id, int code, const std::string& msg) { result(o, id, "{\"error\":{\"code\":" + std::to_string(code) + ",\"message\":" + q(msg) + "}}"); }
    void notify(std::ostream& o, const std::string& method, const std::string& params) { send(o, "{\"jsonrpc\":\"2.0\",\"method\":" + q(method) + ",\"params\":" + params + "}"); }
    std::vector<Symbol> symbols(const std::string& u) {
        std::vector<Symbol> out; auto it = docs.find(u); if (it == docs.end()) return out;
        try { collect(parse(lex(it->second)).statements, out); } catch (...) {} return out;
    }
    void publish(std::ostream& o, const std::string& u) {
        auto it = docs.find(u); if (it == docs.end()) return; std::string d;
        try { auto p = parse(lex(it->second)); try { types::check(p); } catch (const RuntimeError& e) { d = diag(e); } }
        catch (const RuntimeError& e) { d = diag(e); } catch (const std::exception& e) { RuntimeError x(e.what()); d = diag(x); }
        notify(o, "textDocument/publishDiagnostics", "{\"uri\":" + q(u) + ",\"diagnostics\":[" + d + "]}");
    }
public:
    bool handle(std::ostream& o, const Message& m) {
        if (m.method == "initialize") { result(o, m.id, "{\"capabilities\":{\"textDocumentSync\":1,\"completionProvider\":{\"triggerCharacters\":[\".\"]},\"hoverProvider\":true,\"definitionProvider\":true,\"referencesProvider\":true,\"renameProvider\":true,\"documentFormattingProvider\":true}}"); return true; }
        if (m.method == "initialized" || m.method == "$/cancelRequest") return true;
        if (m.method == "shutdown") { result(o, m.id, "null"); return true; }
        if (m.method == "exit") return false;
        if (m.method == "textDocument/didOpen") { auto u = field_uri(m.params); auto text = str_field(m.params, "text"); docs[u] = text; publish(o, u); return true; }
        if (m.method == "textDocument/didChange") { auto u = field_uri(m.params); auto text = str_field(m.params, "text"); if (!text.empty()) docs[u] = text; publish(o, u); return true; }
        auto u = field_uri(m.params); if (u.empty() && docs.size() == 1) u = docs.begin()->first; auto it = docs.find(u); std::string src = it == docs.end() ? "" : it->second;
        int line = int_field(m.params, "line"), col = int_field(m.params, "character"); std::string w = word_at(src, line, col); auto ss = symbols(u);
        Symbol* found = nullptr; for (auto& s : ss) if (s.name == w) { found = &s; break; }
        if (m.method == "textDocument/completion") { std::string items; for (const auto& s : ss) { if (!items.empty()) items += ','; items += "{\"label\":" + q(s.name) + ",\"detail\":" + q(s.detail) + "}"; } result(o, m.id, "{\"isIncomplete\":false,\"items\":[" + items + "]}"); return true; }
        if (m.method == "textDocument/hover") { if (!found) result(o, m.id, "null"); else result(o, m.id, "{\"contents\":{\"kind\":\"markdown\",\"value\":" + q("Ternet " + found->detail + " `" + found->name + "`") + "}}"); return true; }
        if (m.method == "textDocument/definition") { if (!found) result(o, m.id, "null"); else result(o, m.id, "{\"uri\":" + q(u) + ",\"range\":" + position_range(found->pos) + "}"); return true; }
        if (m.method == "textDocument/references") {
            std::string a; size_t p = 0; while (!w.empty() && (p = src.find(w, p)) != std::string::npos) { size_t line0 = 0, ls = std::string::npos; for (size_t i=0;i<p;++i) if(src[i]=='\n') ++line0; ls=src.rfind('\n',p); int c=(int)(p-(ls==std::string::npos?0:ls+1)); if(!a.empty())a+=','; a+="{\"uri\":"+q(u)+",\"range\":{\"start\":{\"line\":"+std::to_string(line0)+",\"character\":"+std::to_string(c)+"},\"end\":{\"line\":"+std::to_string(line0)+",\"character\":"+std::to_string(c+(int)w.size())+"}}}"; p += w.size(); } result(o,m.id,"["+a+"]"); return true;
        }
        if (m.method == "textDocument/rename") {
            auto nn = str_field(m.params, "newName"); if (w.empty() || nn.empty()) { error(o,m.id,-32602,"rename requires a symbol and newName"); return true; }
            std::string edits; size_t p=0; while((p=src.find(w,p))!=std::string::npos){size_t line0=0,ls=std::string::npos;for(size_t i=0;i<p;++i)if(src[i]=='\n')++line0;ls=src.rfind('\n',p);int c=(int)(p-(ls==std::string::npos?0:ls+1));if(!edits.empty())edits+=',';edits+="{\"range\":{\"start\":{\"line\":"+std::to_string(line0)+",\"character\":"+std::to_string(c)+"},\"end\":{\"line\":"+std::to_string(line0)+",\"character\":"+std::to_string(c+(int)w.size())+"}},\"newText\":"+q(nn)+"}";p+=w.size();} result(o,m.id,"{\"changes\":{"+q(u)+":["+edits+"]}}"); return true;
        }
        if (m.method == "textDocument/formatting") { try { result(o,m.id,"[{\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":100000,\"character\":0}},\"newText\":"+q(format_source(src))+"}]"); } catch(const std::exception&e){error(o,m.id,-32602,e.what());} return true; }
        if (!m.id.empty()) error(o,m.id,-32601,"Method not supported: "+m.method); return true;
    }
};

bool read_msg(std::istream& in, Message& m) {
    std::string line, headers; while (std::getline(in,line)) { if(!line.empty()&&line.back()=='\r') line.pop_back(); if(line.empty()) break; headers += line + '\n'; }
    if(headers.empty() && !in) return false; auto p=headers.find("Content-Length:"); if(p==std::string::npos)return false; p+=15; while(p<headers.size()&&headers[p]==' ')++p; size_t n=0;while(p<headers.size()&&std::isdigit((unsigned char)headers[p]))n=n*10+headers[p++]-'0'; std::string body(n,'\0'); in.read(body.data(),(std::streamsize)n); if(in.gcount()!=(std::streamsize)n)return false; m.id=str_field(body,"id");m.method=str_field(body,"method");auto pp=body.find("\"params\"");m.params=pp==std::string::npos?"{}":body.substr(pp);return true;
}
}
int run(std::istream& in, std::ostream& out) { Server s; Message m; while(read_msg(in,m)) if(!s.handle(out,m)) break; return 0; }
} // namespace ternet::lsp
