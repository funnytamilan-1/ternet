#pragma once
#include <cstddef>

namespace ternet {
// V1 runtime currently relies on RAII/value ownership. This interface is reserved
// for the tracing collector used by heap-backed objects in the next runtime layer.
class GarbageCollector {
public:
    void collect() noexcept {}
    std::size_t live_objects() const noexcept { return 0; }
};
}
