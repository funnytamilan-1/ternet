#include "ternet_async.hpp"

#include <cassert>
#include <chrono>
#include <thread>

using namespace ternet;

int main() {
    AsyncRuntime runtime;

    const auto id = runtime.spawn([](std::atomic_bool& cancelled) -> Value {
        for (int i = 0; i < 100; ++i) {
            if (cancelled.load(std::memory_order_acquire)) {
                throw RuntimeError("cancelled", {1, 1}, "E4001");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return Value(static_cast<std::int64_t>(42));
    });

    while (!runtime.is_ready(id)) {
        assert(runtime.active_tasks() == 1);
        break;
    }
    assert(runtime.await(id).str() == "42");
    assert(runtime.active_tasks() == 0);

    const auto cancelled = runtime.spawn([](std::atomic_bool& stop) -> Value {
        while (!stop.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        throw RuntimeError("cancelled", {1, 1}, "E4001");
    });
    assert(runtime.cancel(cancelled));
    bool cancelled_error = false;
    try {
        (void)runtime.await(cancelled);
    } catch (const RuntimeError& e) {
        cancelled_error = e.code == "E4001";
    }
    assert(cancelled_error);

    ValueChannel channel(1);
    assert(channel.send(Value(static_cast<std::int64_t>(7))));
    Value received;
    assert(channel.receive(received));
    assert(received.str() == "7");
    channel.close();
    assert(channel.closed());
    assert(!channel.send(Value(static_cast<std::int64_t>(8))));

    return 0;
}
