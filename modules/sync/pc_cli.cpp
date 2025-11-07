#include "pc_cli.h"
#include "bounded_buffer.h"
#include <sstream>
#include <atomic>
#include <memory>

namespace
{
    // Global buffer para simular Productor–Consumidor
    std::unique_ptr<BoundedBuffer<int>> gbuf(new BoundedBuffer<int>(5));
    std::atomic<int> next_value{1};
}

namespace pc_cli
{

    void init(size_t capacity)
    {
        if (capacity == 0)
            capacity = 1;
        gbuf.reset(new BoundedBuffer<int>(capacity));
        next_value = 1;
    }

    std::string produce(std::optional<int> value)
    {
        int v = value.has_value() ? *value : next_value++;
        bool ok = gbuf->try_produce(v); // no bloquea
        std::ostringstream os;
        if (!ok)
        {
            os << "[pc] buffer lleno (size=" << gbuf->size()
               << "/" << gbuf->capacity() << ")";
        }
        else
        {
            os << "[pc] produced " << v
               << " (size=" << gbuf->size()
               << "/" << gbuf->capacity() << ")";
        }
        return os.str();
    }

    std::string consume()
    {
        auto got = gbuf->try_consume();
        std::ostringstream os;
        if (got.has_value())
        {
            os << "[pc] consumed " << *got
               << " (size=" << gbuf->size()
               << "/" << gbuf->capacity() << ")";
        }
        else
        {
            os << "[pc] buffer vacío";
        }
        return os.str();
    }

    std::string stat()
    {
        std::ostringstream os;
        os << "[pc] size=" << gbuf->size()
           << "/" << gbuf->capacity()
           << " produced=" << gbuf->totalProduced()
           << " consumed=" << gbuf->totalConsumed();
        return os.str();
    }

}
