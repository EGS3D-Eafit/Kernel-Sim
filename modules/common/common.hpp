#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace os
{
    namespace sync
    {

        using Clock = std::chrono::steady_clock;
        using ms = std::chrono::milliseconds;

    }
}
