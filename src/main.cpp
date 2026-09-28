#include "ternet.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: tnc <file.trn>\n";
        return 1;
    }

    const std::string path = argv[1];
    if (path.size() < 4 || path.substr(path.size() - 4) != ".trn") {
        std::cerr << "Ternet error: source file must use .trn extension\n";
        return 1;
    }

    std::ifstream file(path);
    if (!file) {
        std::cerr << "Ternet error: cannot open '" << path << "'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    try {
        const auto tokens = ternet::lex(buffer.str());
        const auto program = ternet::parse(tokens);
        const auto bytecode = ternet::compile(program);
        ternet::VM vm;
        vm.run(bytecode);
    } catch (const std::exception& error) {
        std::cerr << "Ternet error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
