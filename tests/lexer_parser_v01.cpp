#include "ternet.hpp"
#include <cassert>
#include <string>

using namespace ternet;

static void test_lexer_tokens() {
    const auto tokens = lex(R"(
        // line comment
        let answer = 42;
        let ratio = 3.14;
        tnprint("hello\nworld");
        /* block
           comment */
        if {answer >= 40 && answer != 0};
    )");

    assert(tokens.size() > 10);
    assert(tokens[0].type == TokenType::Keyword && tokens[0].text == "let");
    assert(tokens[1].type == TokenType::Identifier && tokens[1].text == "answer");
    assert(tokens[2].type == TokenType::Op && tokens[2].text == "=");
    assert(tokens[3].type == TokenType::Number && tokens[3].text == "42");
}

static void test_parser_smoke() {
    const auto program = parse(lex(R"(
        let answer = 42;
        tnprint(answer);
        if {answer >= 40};
    )"));
    assert(program.statements.size() == 3);
    assert(program.statements[0]->kind == Stmt::Let);
    assert(program.statements[1]->kind == Stmt::Print);
    assert(program.statements[2]->kind == Stmt::If);
}

static void test_lexer_errors() {
    bool failed = false;
    try { (void)lex("/* never closes"); } catch (const RuntimeError&) { failed = true; }
    assert(failed);

    failed = false;
    try { (void)lex("\"never closes"); } catch (const RuntimeError&) { failed = true; }
    assert(failed);

    failed = false;
    try { (void)lex("12abc"); } catch (const RuntimeError&) { failed = true; }
    assert(failed);
}

int main() {
    test_lexer_tokens();
    test_parser_smoke();
    test_lexer_errors();
    return 0;
}
