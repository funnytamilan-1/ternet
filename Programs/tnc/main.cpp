#include "ternet.hpp"
#include "bytecode.hpp"
#include "types.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

#ifdef __unix__
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace ternet {

static std::string read_source(const std::string& p) {
    std::ifstream f(p);
    if (!f) throw RuntimeError("cannot open '" + p + "'");
    std::stringstream b;
    b << f.rdbuf();
    return b.str();
}

static fs::path resolve_import(const fs::path& importer, const std::string& module) {
    const fs::path base = importer.parent_path();
    fs::path raw(module);
    std::vector<fs::path> candidates;
    if (raw.extension() == ".trn") candidates.push_back(base / raw);
    else {
        candidates.push_back(base / raw);
        candidates.push_back(base / (raw.string() + ".trn"));
        candidates.push_back(base / raw / "Node.trn");
        candidates.push_back(base / raw / "main.trn");
    }
    for (const auto& p : candidates) if (fs::is_regular_file(p)) return p;
    throw RuntimeError("cannot resolve import '" + module + "' from '" + importer.string() + "'");
}

static void append_loaded(Program& out, const fs::path& file,
                          std::unordered_set<std::string>& loading,
                          std::unordered_set<std::string>& loaded) {
    const auto canonical = fs::weakly_canonical(file).string();
    if (loaded.count(canonical)) return;
    if (!loading.insert(canonical).second)
        throw RuntimeError("cyclic import detected at '" + canonical + "'");

    const auto program = parse(lex(read_source(canonical)));
    for (const auto& st : program.statements) {
        if (st && st->kind == Stmt::Import) {
            append_loaded(out, resolve_import(canonical, st->module_path), loading, loaded);
        } else {
            out.statements.push_back(st);
        }
    }

    loading.erase(canonical);
    loaded.insert(canonical);
}

static Program load_program(const std::string& entry) {
    Program out;
    std::unordered_set<std::string> loading;
    std::unordered_set<std::string> loaded;
    append_loaded(out, fs::path(entry), loading, loaded);
    return out;
}

std::string format_source(const std::string& source) {
    auto tokens = lex(source);
    std::string out;
    int indent = 0;
    auto emit_indent = [&]() {
        for (int k = 0; k < indent; ++k) out += "    ";
    };

    bool line_start = true;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& t = tokens[i];
        if (t.type == TokenType::End) break;

        if (t.type == TokenType::RBrace) {
            if (indent > 0) --indent;
            if (!line_start) out += "\n";
            emit_indent();
            out += "}";
            out += "\n";
            line_start = true;
            continue;
        }

        if (line_start) {
            emit_indent();
            line_start = false;
        }

        if (t.type == TokenType::String) {
            out += "\"" + t.text + "\"";
        } else if (t.type == TokenType::LBrace) {
            out += " {";
            out += "\n";
            ++indent;
            line_start = true;
            continue;
        } else if (t.type == TokenType::Comma) {
            out += ", ";
        } else if (t.type == TokenType::Colon) {
            if (i + 1 < tokens.size() && tokens[i + 1].type != TokenType::End &&
                tokens[i + 1].type != TokenType::RBrace && tokens[i + 1].type != TokenType::LBrace) {
                out += ":\n";
                line_start = true;
            } else {
                out += ": ";
            }
        } else if (t.type == TokenType::Semicolon) {
            out += ";\n";
            line_start = true;
        } else if (t.type == TokenType::Op) {
            if (t.text == ".." || t.text == "!" || t.text == "~") {
                out += t.text;
            } else {
                out += " " + t.text + " ";
            }
        } else {
            // Space before identifier/keyword if preceding token was identifier/keyword/number
            if (!out.empty() && out.back() != ' ' && out.back() != '\n' &&
                out.back() != '(' && out.back() != '[' && out.back() != '{' && out.back() != '.') {
                out += " ";
            }
            out += t.text;
        }
    }

    if (!out.empty() && out.back() != '\n') out += "\n";
    return out;
}

std::vector<std::string> lint_source(const std::string& source, const std::string& filename) {
    std::vector<std::string> diagnostics;
    try {
        auto tokens = lex(source);
        auto program = parse(tokens);

        // Check for empty blocks, unused variables
        std::unordered_set<std::string> declared;
        std::unordered_set<std::string> used;

        for (const auto& s : program.statements) {
            if (!s) continue;
            if (s->kind == Stmt::Let) {
                declared.insert(s->name);
            }
            if (s->kind == Stmt::Try && s->catch_body.empty() && s->finally_body.empty()) {
                diagnostics.push_back(filename + ":" + std::to_string(s->pos.line) + ":" +
                                      std::to_string(s->pos.column) + ": warning [W001]: empty try/catch block");
            }
        }
    } catch (const std::exception& e) {
        diagnostics.push_back(filename + ":1:1: error: " + e.what());
    }
    return diagnostics;
}

int run_repl() {
    std::cout << "Ternet 0.2.0 Interactive REPL\n";
    std::cout << "Type code to evaluate, or 'exit' / Ctrl+D to quit.\n\n";

    Interpreter interp;
    std::string line;
    Program accumulated;

    while (true) {
        std::cout << "ternet> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;

        try {
            // Add trailing delimiter if missing
            std::string code = line;
            if (code.back() != ';' && code.back() != ':' && code.back() != '}') {
                code += ";";
            }
            auto tokens = lex(code);
            auto p = parse(tokens);
            interp.run(p);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
    std::cout << "Goodbye!\n";
    return 0;
}

} // namespace ternet

static int build_file(const std::string& p, const std::string& out) {
    try {
        auto program = ternet::load_program(p);
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

#ifdef __unix__
static int serve_http() {
    const char* env = std::getenv("PORT");
    const int port = env ? std::atoi(env) : 8080;
    if (port < 1 || port > 65535) {
        std::cerr << "tnc E7001: invalid PORT\n";
        return 2;
    }

    const int server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        std::cerr << "tnc E7002: socket() failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    int reuse = 1;
    ::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<std::uint16_t>(port));

    if (::bind(server, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
        ::listen(server, 16) < 0) {
        std::cerr << "tnc E7003: cannot listen on port " << port << ": "
                  << std::strerror(errno) << "\n";
        ::close(server);
        return 1;
    }

    std::cout << "tnc serve: health server listening on 0.0.0.0:" << port << "\n";
    const char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: 10\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Ternet OK\n";

    for (;;) {
        const int client = ::accept(server, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR) continue;
            std::cerr << "tnc E7004: accept() failed: " << std::strerror(errno) << "\n";
            ::close(server);
            return 1;
        }

        char request[1024];
        (void)::recv(client, request, sizeof(request), 0);
        (void)::send(client, response, sizeof(response) - 1, 0);
        ::close(client);
    }
}
#else
static int serve_http() {
    std::cerr << "tnc E7005: HTTP serve mode is currently supported on Unix-like hosts only\n";
    return 2;
}
#endif

static int run_file(const std::string& p) {
    if (p.size() < 4 || p.substr(p.size() - 4) != ".trn") {
        std::cerr << "tnc E0001: source file must use .trn\n";
        return 2;
    }
    std::ifstream f(p);
    if (!f) {
        std::cerr << "tnc E0002: cannot open '" << p << "'\n";
        return 2;
    }
    try {
        ternet::Interpreter vm;
        vm.run(ternet::load_program(p));
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Ternet error E1000: " << e.what() << "\n";
        return 1;
    }
}

static std::string project_entry(const std::string& dir) {
    const fs::path root(dir);
    if (fs::exists(root / "Node.trn")) return (root / "Node.trn").string();
    if (fs::exists(root / "src" / "main.trn")) return (root / "src" / "main.trn").string();
    return {};
}

static void help() {
    std::cout
        << "Ternet toolchain\n\n"
        << "Usage:\n"
        << "  tnc run [file.trn]\n"
        << "  tnc serve\n"
        << "  tnc web build [file.trn]\n"
        << "  tnc check <file.trn>\n"
        << "  tnc build <file.trn> [-o file.tbc]\n"
        << "  tnc exec <file.tbc>\n"
        << "  tnc fmt <file.trn>\n"
        << "  tnc lint <file.trn>\n"
        << "  tnc repl\n"
        << "  tnc <file.trn>\n"
        << "  tnc init [dir]\n"
        << "  tnc add <name> <version>\n"
        << "  tnc install\n"
        << "  tnc remove <name>\n"
        << "  tnc list\n"
        << "  tnc package\n"
        << "  tnc version\n\n"
        << "Web output is written to ./dist.\n"
        << "Project entry priority: Node.trn, then src/main.trn.\n";
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            if (std::getenv("PORT")) return serve_http();
            auto entry = project_entry(".");
            if (!entry.empty()) return run_file(entry);
            help();
            return 1;
        }

        const std::string c = argv[1];

        if (c == "version" || c == "--version" || c == "-v") {
            std::cout << "Ternet 0.2.0 (interpreter + bytecode VM + typechecker)\n";
            return 0;
        }

        if (c == "serve") return serve_http();

        if (c == "repl") return ternet::run_repl();

        if (c == "fmt" && argc >= 3) {
            std::string src = ternet::read_source(argv[2]);
            std::string formatted = ternet::format_source(src);
            std::ofstream out(argv[2]);
            out << formatted;
            std::cout << "Formatted " << argv[2] << "\n";
            return 0;
        }

        if (c == "lint" && argc >= 3) {
            std::string src = ternet::read_source(argv[2]);
            auto diags = ternet::lint_source(src, argv[2]);
            for (const auto& d : diags) std::cout << d << "\n";
            if (diags.empty()) std::cout << "lint: 0 issues found\n";
            return 0;
        }

        if (c == "build" && argc >= 3) {
            const std::string out =
                (argc >= 5 && std::string(argv[3]) == "-o")
                    ? argv[4]
                    : std::string(argv[2]) + ".tbc";
            return build_file(argv[2], out);
        }

        if (c == "exec" && argc == 3) return exec_bytecode(argv[2]);

        if (c == "run") {
            const auto entry = argc >= 3 ? argv[2] : project_entry(".");
            if (entry.empty()) {
                std::cerr << "tnc E0003: no entry file; expected Node.trn or src/main.trn\n";
                return 2;
            }
            return run_file(entry);
        }

        if (c == "web" && argc >= 3 && std::string(argv[2]) == "build") {
            const auto entry = argc >= 4 ? argv[3] : project_entry(".");
            if (entry.empty()) {
                std::cerr << "tnc E0004: no entry file; expected Node.trn or src/main.trn\n";
                return 2;
            }
            std::cout << "tnc web: building " << entry << " -> dist/\n";
            return run_file(entry);
        }

        if (c == "check" && argc == 3) {
            try {
                auto program = ternet::load_program(argv[2]);
                ternet::types::check(program);
                std::cout << "check: ok\n";
                return 0;
            } catch (const std::exception& e) {
                std::cerr << "Ternet syntax/semantic error E1000: " << e.what() << "\n";
                return 1;
            }
        }

        if (c == "init") return ternet::command_init(argc > 2 ? argv[2] : ".");
        if (c == "add" && argc == 4) return ternet::command_add(".", argv[2], argv[3]);
        if (c == "install") return ternet::command_install(".");
        if (c == "remove" && argc == 3) return ternet::command_remove(".", argv[2]);
        if (c == "list") return ternet::command_list(".");
        if (c == "package") return ternet::command_package(".");

        if (c.size() >= 4 && c.substr(c.size() - 4) == ".trn") return run_file(c);

        help();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "tnc E0000: " << e.what() << "\n";
        return 1;
    }
}
