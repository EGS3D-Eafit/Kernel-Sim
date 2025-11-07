#pragma once
#include <mutex>
#include <condition_variable>
#include <chrono>

// Simple counting semaphore (binary when initialized to 1).
// Thread-safe, C++17.
class Semaphore
{
public:
    explicit Semaphore(int initial = 0) : count_(initial) {}

    // Blocking acquire: waits until count_ > 0, then decrements.
    void acquire()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&]
                 { return count_ > 0; });
        --count_;
    }

    // Non-blocking acquire: returns false if count_ == 0.
    bool try_acquire()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (count_ <= 0)
            return false;
        --count_;
        return true;
    }

    // Timed acquire: waits up to 'dur'; returns true if acquired.
    template <class Rep, class Period>
    bool acquire_for(const std::chrono::duration<Rep, Period> &dur)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!cv_.wait_for(lock, dur, [&]
                          { return count_ > 0; }))
            return false;
        --count_;
        return true;
    }

    // Release increments the count and notifies one waiter.
    void release()
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            ++count_;
        }
        cv_.notify_one();
    }

    // Diagnostic only (not linearizable across threads).
    int value() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return count_;
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    int count_;
};
