#include "lsp.hpp"
#include <cassert>
#include <sstream>
#include <string>

static void send(std::ostringstream& in, const std::string& json) {
    in << "Content-Length: " << json.size() << "\r\n\r\n" << json;
}

int main() {
    std::ostringstream wire;
    send(wire, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}\n");
    send(wire, "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///test.trn\",\"text\":\"let answer = 42;\\nprint(answer);\"}}}");
    send(wire, "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"file:///test.trn\"},\"position\":{\"line\":1,\"character\":6}}}");
    send(wire, "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"file:///test.trn\"},\"position\":{\"line\":1,\"character\":6}}}");
    send(wire, "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"file:///test.trn\"},\"position\":{\"line\":1,\"character\":6}}}");
    send(wire, "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"textDocument/references\",\"params\":{\"textDocument\":{\"uri\":\"file:///test.trn\"},\"position\":{\"line\":1,\"character\":6}}}");
    send(wire, "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"textDocument/rename\",\"params\":{\"textDocument\":{\"uri\":\"file:///test.trn\"},\"position\":{\"line\":1,\"character\":6},\"newName\":\"result\"}}");
    send(wire, "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"textDocument/formatting\",\"params\":{\"textDocument\":{\"uri\":\"file:///test.trn\"}}}");
    send(wire, "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"shutdown\",\"params\":null}");
    send(wire, "{\"jsonrpc\":\"2.0\",\"method\":\"exit\"}");

    std::istringstream input(wire.str());
    std::ostringstream output;
    assert(ternet::lsp::run(input, output) == 0);
    const std::string out = output.str();
    assert(out.find("\"id\":1") != std::string::npos);
    assert(out.find("textDocument/publishDiagnostics") != std::string::npos);
    assert(out.find("\"id\":2") != std::string::npos);
    assert(out.find("answer") != std::string::npos);
    assert(out.find("\"id\":3") != std::string::npos);
    assert(out.find("\"id\":4") != std::string::npos);
    assert(out.find("\"id\":5") != std::string::npos);
    assert(out.find("\"id\":6") != std::string::npos);
    assert(out.find("\"id\":7") != std::string::npos);
    assert(out.find("\"id\":8") != std::string::npos);
    return 0;
}
