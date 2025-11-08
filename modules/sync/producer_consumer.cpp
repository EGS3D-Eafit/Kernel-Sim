#include "producer_consumer.hpp"

namespace os
{
    namespace sync
    {

        void ProducerConsumer::start(std::size_t producers, std::size_t consumers, std::size_t buffer_cap,
                                     ms prod_delay, ms cons_delay, bool verbose)
        {
            stop(); // por si estaba corriendo

            {
                std::lock_guard<std::mutex> lk(mtx_);
                buffer_ = std::queue<std::size_t>();
                buffer_cap_ = buffer_cap;
            }
            produced_ = 0;
            consumed_ = 0;
            item_counter_ = 0;
            max_buffer_seen_ = 0;

            running_ = true;
            threads_.reserve(producers + consumers);

            for (std::size_t i = 0; i < producers; ++i)
            {
                threads_.emplace_back(&ProducerConsumer::producer_loop, this, i + 1, prod_delay, verbose);
            }
            for (std::size_t j = 0; j < consumers; ++j)
            {
                threads_.emplace_back(&ProducerConsumer::consumer_loop, this, j + 1, cons_delay, verbose);
            }
        }

        void ProducerConsumer::stop()
        {
            if (!running_)
                return;
            running_ = false;
            cv_not_empty_.notify_all();
            cv_not_full_.notify_all();
            for (auto &t : threads_)
                if (t.joinable())
                    t.join();
            threads_.clear();
        }

        PCStats ProducerConsumer::stats() const
        {
            std::lock_guard<std::mutex> lk(mtx_);
            PCStats s;
            s.produced = produced_.load();
            s.consumed = consumed_.load();
            s.max_buffer = max_buffer_seen_.load();
            s.last_item = item_counter_.load();
            return s;
        }

        void ProducerConsumer::producer_loop(std::size_t id, ms delay, bool verbose)
        {
            while (running_)
            {
                std::this_thread::sleep_for(delay); // simula producción

                std::unique_lock<std::mutex> lk(mtx_);
                cv_not_full_.wait(lk, [&]
                                  { return !running_ || buffer_.size() < buffer_cap_; });
                if (!running_)
                    break;

                std::size_t next = ++item_counter_;
                buffer_.push(next);
                ++produced_;

                if (buffer_.size() > max_buffer_seen_)
                    max_buffer_seen_ = buffer_.size();
                if (verbose)
                {
                    std::cout << "[P" << id << "] produce item " << next
                              << " | buffer=" << buffer_.size() << "/" << buffer_cap_ << "\n";
                }

                cv_not_empty_.notify_one();
            }
        }

        void ProducerConsumer::consumer_loop(std::size_t id, ms delay, bool verbose)
        {
            while (running_)
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_not_empty_.wait(lk, [&]
                                   { return !running_ || !buffer_.empty(); });
                if (!running_)
                    break;

                auto item = buffer_.front();
                buffer_.pop();
                ++consumed_;

                if (verbose)
                {
                    std::cout << "    [C" << id << "] consume item " << item
                              << " | buffer=" << buffer_.size() << "/" << buffer_cap_ << "\n";
                }

                cv_not_full_.notify_one();
                lk.unlock();

                std::this_thread::sleep_for(delay); // simula consumo
            }
        }

    }
}
