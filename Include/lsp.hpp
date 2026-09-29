#pragma once

#include <istream>
#include <ostream>

namespace ternet::lsp {

// Run a Language Server Protocol endpoint over JSON-RPC stdio.
// Returns 0 after a clean shutdown/exit request.
int run(std::istream& in, std::ostream& out);

} // namespace ternet::lsp
