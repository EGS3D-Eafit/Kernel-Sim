#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

// Thread-safe bounded buffer suitable for Producer-Consumer (multi-producer/consumer).
// Pensado para dos modos de uso:
//  - Con hilos: usa produce() / consume() (bloqueantes).
//  - Con CLI paso-a-paso: usa try_produce() / try_consume() (no bloqueantes).
template <typename T>
class BoundedBuffer
{
public:
    explicit BoundedBuffer(size_t capacity) : capacity_(capacity) {}

    // Bloquea cuando el buffer está lleno
    void produce(const T &item)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        not_full_.wait(lock, [&]
                       { return queue_.size() < capacity_; });
        queue_.push(item);
        ++produced_;
        lock.unlock();
        not_empty_.notify_one();
    }

    // Bloquea cuando el buffer está vacío
    T consume()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock, [&]
                        { return !queue_.empty(); });
        T item = queue_.front();
        queue_.pop();
        ++consumed_;
        lock.unlock();
        not_full_.notify_one();
        return item;
    }

    // No bloquea: falla si lleno
    bool try_produce(const T &item)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (queue_.size() >= capacity_)
            return false;
        queue_.push(item);
        ++produced_;
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    // No bloquea: devuelve nullopt si vacío
    std::optional<T> try_consume()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (queue_.empty())
            return std::nullopt;
        T item = queue_.front();
        queue_.pop();
        ++consumed_;
        lock.unlock();
        not_full_.notify_one();
        return item;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    size_t capacity() const { return capacity_; }

    size_t totalProduced() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return produced_;
    }
    size_t totalConsumed() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return consumed_;
    }

    void clear()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        std::queue<T> empty;
        std::swap(queue_, empty);
        produced_ = consumed_ = 0;
        lock.unlock();
        not_full_.notify_all();
    }

    struct Snapshot
    {
        size_t size;
        size_t capacity;
        size_t produced;
        size_t consumed;
    };

    Snapshot snapshot() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return Snapshot{queue_.size(), capacity_, produced_, consumed_};
    }

private:
    size_t capacity_;
    mutable std::mutex mtx_;
    std::condition_variable not_empty_, not_full_;
    std::queue<T> queue_;
    size_t produced_ = 0;
    size_t consumed_ = 0;
};
