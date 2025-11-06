#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

// Thread-safe bounded buffer suitable for Producer-Consumer (multiple producers/consumers).
template <typename T>
class BoundedBuffer {
public:
    explicit BoundedBuffer(size_t capacity) : capacity_(capacity) {}

    // Blocks when buffer full
    void produce(const T& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        not_full_.wait(lock, [&]{ return queue_.size() < capacity_; });
        queue_.push(item);
        ++produced_;
        lock.unlock();
        not_empty_.notify_one();
    }

    // Blocks when buffer empty
    T consume() {
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock, [&]{ return !queue_.empty(); });
        T item = queue_.front();
        queue_.pop();
        ++consumed_;
        lock.unlock();
        not_full_.notify_one();
        return item;
    }

    // Non-blocking try-consume (useful for CLI step-by-step)
    std::optional<T> try_consume() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) return std::nullopt;
        T item = queue_.front();
        queue_.pop();
        ++consumed_;
        not_full_.notify_one();
        return item;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    size_t capacity() const { return capacity_; }

    size_t totalProduced() const { return produced_; }
    size_t totalConsumed() const { return consumed_; }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        std::queue<T> empty;
        std::swap(queue_, empty);
        produced_ = consumed_ = 0;
    }

private:
    size_t capacity_;
    mutable std::mutex mtx_;
    std::condition_variable not_empty_, not_full_;
    std::queue<T> queue_;
    size_t produced_ = 0;
    size_t consumed_ = 0;
};
