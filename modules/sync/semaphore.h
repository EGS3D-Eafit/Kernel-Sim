#pragma once
#include <mutex>
#include <condition_variable>

// Simple counting semaphore (can be used as binary semaphore when initialized to 1)
class Semaphore {
public:
    explicit Semaphore(int initial = 0) : count_(initial) {}

    void acquire() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&]{ return count_ > 0; });
        --count_;
    }

    void release() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            ++count_;
        }
        cv_.notify_one();
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;
};
