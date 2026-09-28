#include "ternet.hpp"
#include "bytecode.hpp"
#include "types.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef __unix__
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static std::string read_source(const std::string& p) {
    std::ifstream f(p);
    if (!f) throw ternet::RuntimeError("cannot open '" + p + "'");
    std::stringstream b;
    b << f.rdbuf();
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
    std::stringstream b;
    b << f.rdbuf();
    try {
        ternet::Interpreter vm;
        vm.run(ternet::parse(ternet::lex(b.str())));
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

        if (c == "version") {
            std::cout << "Ternet 0.2.0-dev (reference VM + bytecode VM)\n";
            return 0;
        }

        if (c == "serve") return serve_http();

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
            std::ifstream f(argv[2]);
            if (!f) throw ternet::RuntimeError("cannot open '" + std::string(argv[2]) + "'");
            std::stringstream b;
            b << f.rdbuf();
            try {
                auto program = ternet::parse(ternet::lex(b.str()));
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
