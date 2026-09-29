#include "ternet_async.hpp"

#include <chrono>
#include <stdexcept>

namespace ternet {

AsyncRuntime::~AsyncRuntime() {
    // shared_future destruction waits only for its shared state to be released;
    // the worker itself is owned by std::async and is joined by its shared state.
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.clear();
}

AsyncRuntime::TaskId AsyncRuntime::spawn(Task task) {
    if (!task) throw std::invalid_argument("async task must be callable");

    auto state = std::make_shared<TaskState>();
    state->future = std::async(std::launch::async,
        [state, task = std::move(task)]() mutable -> Value {
            if (state->cancelled.load(std::memory_order_acquire)) {
                throw RuntimeError("async task cancelled", {1, 1}, "E4001");
            }
            return task(state->cancelled);
        }).share();

    std::lock_guard<std::mutex> lock(mutex_);
    const TaskId id = next_id_++;
    tasks_.emplace(id, std::move(state));
    return id;
}

Value AsyncRuntime::await(TaskId id) {
    std::shared_ptr<TaskState> state;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(id);
        if (it == tasks_.end()) {
            throw RuntimeError("unknown async task " + std::to_string(id), {1, 1}, "E4002");
        }
        state = it->second;
    }

    // Exceptions from the worker propagate through future::get().
    Value result = state->future.get();

    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.erase(id);
    return result;
}

bool AsyncRuntime::cancel(TaskId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(id);
    if (it == tasks_.end()) return false;
    it->second->cancelled.store(true, std::memory_order_release);
    return true;
}

bool AsyncRuntime::is_ready(TaskId id) const {
    std::shared_ptr<TaskState> state;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(id);
        if (it == tasks_.end()) return false;
        state = it->second;
    }
    return state->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

std::size_t AsyncRuntime::active_tasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

ValueChannel::ValueChannel(std::size_t capacity) : capacity_(capacity) {}

bool ValueChannel::send(Value value) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (capacity_ != 0) {
        not_full_.wait(lock, [this] { return closed_ || queue_.size() < capacity_; });
    } else {
        // Unbounded channel: only closure can stop a sender.
        if (closed_) return false;
    }
    if (closed_) return false;
    queue_.push_back(std::move(value));
    lock.unlock();
    not_empty_.notify_one();
    return true;
}

bool ValueChannel::receive(Value& out) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_empty_.wait(lock, [this] { return closed_ || !queue_.empty(); });
    if (queue_.empty()) return false;
    out = std::move(queue_.front());
    queue_.pop_front();
    lock.unlock();
    not_full_.notify_one();
    return true;
}

void ValueChannel::close() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }
    not_empty_.notify_all();
    not_full_.notify_all();
}

bool ValueChannel::closed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

std::size_t ValueChannel::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

} // namespace ternet
