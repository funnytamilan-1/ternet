#pragma once

#include "ternet.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace ternet {

// Runtime-level asynchronous task scheduler. This is intentionally independent
// from the parser so it can be tested deterministically before language syntax
// is wired into the interpreter/bytecode compiler.
class AsyncRuntime {
public:
    using TaskId = std::uint64_t;
    using Task = std::function<Value(std::atomic_bool& cancelled)>;

    AsyncRuntime() = default;
    AsyncRuntime(const AsyncRuntime&) = delete;
    AsyncRuntime& operator=(const AsyncRuntime&) = delete;
    ~AsyncRuntime();

    TaskId spawn(Task task);
    Value await(TaskId id);
    bool cancel(TaskId id);
    bool is_ready(TaskId id) const;
    std::size_t active_tasks() const;

private:
    struct TaskState {
        std::atomic_bool cancelled{false};
        std::shared_future<Value> future;
    };

    mutable std::mutex mutex_;
    std::unordered_map<TaskId, std::shared_ptr<TaskState>> tasks_;
    TaskId next_id_ = 1;
};

// A bounded, typed-at-runtime channel used by async tasks. send() blocks when
// the channel is full; receive() blocks until a value is available or closed.
class ValueChannel {
public:
    explicit ValueChannel(std::size_t capacity = 0);
    ValueChannel(const ValueChannel&) = delete;
    ValueChannel& operator=(const ValueChannel&) = delete;

    bool send(Value value);
    bool receive(Value& out);
    void close();
    bool closed() const;
    std::size_t size() const;

private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<Value> queue_;
    bool closed_ = false;
};

} // namespace ternet
