#include "ternet.hpp"
#include "bytecode.hpp"
#include "types.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace fs = std::filesystem;
static std::string read_source(const std::string& p) {
    std::ifstream f(p);
    if (!f) throw ternet::RuntimeError("cannot open '" + p + "'");
    std::stringstream b; b << f.rdbuf();
    return b.str();
}
static int build_file(const std::string& p, const std::string& out) {
    try {
        auto program = ternet::parse(ternet::lex(read_source(p)));
        auto chunk = ternet::bytecode::compile(program);
        ternet::bytecode::write(chunk, out);
        std::cout << "tnc: compiled " << p << " -> " << out << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Ternet compiler error: " << e.what() << "\n";
        return 1;
    }
}
static int exec_bytecode(const std::string& p) {
    try {
        ternet::bytecode::execute(ternet::bytecode::read(p));
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Ternet VM error: " << e.what() << "\n";
        return 1;
    }
}
static int run_file(const std::string& p) {
    if (p.size() < 4 || p.substr(p.size() - 4) != ".trn") { std::cerr << "tnc E0001: source file must use .trn\n"; return 2; }
    std::ifstream f(p); if (!f) { std::cerr << "tnc E0002: cannot open '" << p << "'\n"; return 2; }
    std::stringstream b; b << f.rdbuf();
    try { ternet::Interpreter vm; vm.run(ternet::parse(ternet::lex(b.str()))); return 0; }
    catch (const std::exception& e) { std::cerr << "Ternet error E1000: " << e.what() << "\n"; return 1; }
}
static std::string project_entry(const std::string& dir) {
    const fs::path root(dir);
    if (fs::exists(root/"Node.trn")) return (root/"Node.trn").string();
    if (fs::exists(root/"src"/"main.trn")) return (root/"src"/"main.trn").string();
    return {};
}
static void help() {
    std::cout << "Ternet toolchain\n\nUsage:\n"
              << "  tnc run [file.trn]\n  tnc web build [file.trn]\n  tnc check <file.trn>\n  tnc build <file.trn> [-o file.tbc]\n  tnc exec <file.tbc>\n  tnc <file.trn>\n"
              << "  tnc init [dir]\n  tnc add <name> <version>\n  tnc install\n"
              << "  tnc remove <name>\n  tnc list\n  tnc package\n  tnc version\n\n"
              << "Web output is written to ./dist.\n"
              << "Project entry priority: Node.trn, then src/main.trn.\n";
}
int main(int argc, char** argv) {
    try {
        if (argc < 2) { auto entry = project_entry("."); if (!entry.empty()) return run_file(entry); help(); return 1; }
        std::string c = argv[1];
        if (c == "version") { std::cout << "Ternet 0.2.0-dev (reference VM)\n"; return 0; }
        if (c == "build" && argc >= 3) { std::string out = (argc >= 5 && std::string(argv[3]) == "-o") ? argv[4] : std::string(argv[2]) + ".tbc"; return build_file(argv[2], out); }\n        if (c == "exec" && argc == 3) return exec_bytecode(argv[2]);\n        if (c == "run") { auto entry = argc >= 3 ? argv[2] : project_entry("."); if (entry.empty()) { std::cerr << "tnc E0003: no entry file; expected Node.trn or src/main.trn\n"; return 2; } return run_file(entry); }
        if (c == "web" && argc >= 3 && std::string(argv[2]) == "build") {
            auto entry = argc >= 4 ? argv[3] : project_entry(".");
            if (entry.empty()) { std::cerr << "tnc E0004: no web entry file; expected Node.trn or src/main.trn\n"; return 2; }
            std::cout << "tnc web: building " << entry << " -> dist/\n";
            return run_file(entry);
        }
        if (c == "check" && argc == 3) {
            std::ifstream f(argv[2]); if (!f) throw ternet::RuntimeError("cannot open '" + std::string(argv[2]) + "'");
            std::stringstream b; b << f.rdbuf();
            try { auto program = ternet::parse(ternet::lex(b.str())); ternet::types::check(program); std::cout << "check: ok\n"; return 0; }
            catch (const std::exception& e) { std::cerr << "Ternet syntax/semantic error E1000: " << e.what() << "\n"; return 1; }
        }
        if (c == "init") return ternet::command_init(argc > 2 ? argv[2] : ".");
        if (c == "add" && argc == 4) return ternet::command_add(".", argv[2], argv[3]);
        if (c == "install") return ternet::command_install(".");
        if (c == "remove" && argc == 3) return ternet::command_remove(".", argv[2]);
        if (c == "list") return ternet::command_list(".");
        if (c == "package") return ternet::command_package(".");
        if (c.size() >= 4 && c.substr(c.size()-4) == ".trn") return run_file(c);
        help(); return 1;
    } catch (const std::exception& e) { std::cerr << "tnc E0000: " << e.what() << "\n"; return 1; }
}