#include "device_queue.hpp"
#include <algorithm>
#include <fstream>

namespace os
{
    namespace io
    {

        void Device::reset()
        {
            params_.clamp();
            now_ = 0;
            incoming_.clear();
            completed_.clear();
            pool_.clear();
            queues_.assign(params_.priority_levels, {});
            current_ = -1;
            busy_ticks_ = 0;
            service_order_.clear();
        }

        void Device::submit(const IORequest &req_in)
        {
            IORequest req = req_in;
            if (req.priority < 0)
                req.priority = 0;
            if (req.priority >= params_.priority_levels)
                req.priority = params_.priority_levels - 1;
            if (req.service < 1)
                req.service = 1;
            req.remaining = req.service;
            incoming_.push_back(Pending{req, -1});
            // mantener incoming ordenado por arrival (opcional)
            std::sort(incoming_.begin(), incoming_.end(),
                      [](const Pending &a, const Pending &b)
                      { return a.io.arrival < b.io.arrival; });
        }

        void Device::tick(int ticks)
        {
            if (ticks <= 0)
                return;
            for (int k = 0; k < ticks; ++k)
            {
                admit_arrivals();
                promote_aging();
                dispatch_if_idle();
                run_one_tick();
                now_++;
            }
        }

        void Device::admit_arrivals()
        {
            // mover solicitudes con arrival <= now a la cola por prioridad
            while (!incoming_.empty() && incoming_.front().io.arrival <= now_)
            {
                Pending p = incoming_.front();
                incoming_.erase(incoming_.begin());

                p.enqueued_at = now_;
                int idx = (int)pool_.size();
                pool_.push_back(p);
                queues_[p.io.priority].push_back(idx);
            }
        }

        void Device::promote_aging()
        {
            if (!params_.aging_enabled)
                return;
            const int A = params_.aging_period;

            for (int pr = 1; pr < params_.priority_levels; ++pr)
            {
                auto &q = queues_[pr];
                for (size_t i = 0; i < q.size();)
                {
                    int idx = q[i];
                    Pending &p = pool_[idx];
                    int waited = (now_ - p.enqueued_at); // tiempo desde que entró a colas
                    if (waited > 0 && waited % A == 0)
                    {
                        // promover a prioridad superior (numérico más chico)
                        int new_pr = pr - 1;
                        p.io.priority = new_pr;
                        // quitar de cola actual
                        q.erase(q.begin() + i);
                        // reencolar al final de la cola superior
                        queues_[new_pr].push_back(idx);
                        // actualizar su "enqueued_at" para resetear el período de aging
                        p.enqueued_at = now_;
                    }
                    else
                    {
                        ++i;
                    }
                }
            }
        }

        void Device::dispatch_if_idle()
        {
            if (!idle())
                return;
            // escoger la cola de mayor prioridad con elementos
            for (int pr = 0; pr < params_.priority_levels; ++pr)
            {
                auto &q = queues_[pr];
                if (!q.empty())
                {
                    int idx = q.front();
                    q.pop_front();
                    current_ = idx;
                    Pending &p = pool_[idx];
                    if (p.io.start < 0)
                    {
                        p.io.start = now_;
                        p.io.wait = p.io.start - p.io.arrival;
                    }
                    // registrar primera vez que empezamos a servir esta solicitud
                    service_order_.push_back(p.io.rid);
                    return;
                }
            }
        }

        void Device::run_one_tick()
        {
            // incrementar espera de todo lo que está en colas
            for (auto &q : queues_)
            {
                for (int idx : q)
                {
                    pool_[idx].io.wait++;
                }
            }

            if (idle())
                return;

            Pending &p = pool_[current_];
            p.io.remaining--;
            busy_ticks_++;

            if (p.io.remaining <= 0)
            {
                p.io.finish = now_ + 1; // termina al final del tick
                finish_current();
            }
        }

        void Device::finish_current()
        {
            if (idle())
                return;
            completed_.push_back(pool_[current_]);
            current_ = -1;
        }

        DeviceStats Device::stats() const
        {
            DeviceStats s{};
            s.total_requests = (int)completed_.size();
            s.total_time = now_;
            s.busy_time = busy_ticks_;
            s.utilization = (s.total_time > 0 ? (double)busy_ticks_ / (double)s.total_time : 0.0);

            int levels = params_.priority_levels;
            s.count_by_prio.assign(levels, 0);
            s.wait_by_prio.assign(levels, 0);

            for (const auto &p : completed_)
            {
                s.total_wait += p.io.wait;
                if (p.io.priority >= 0 && p.io.priority < levels)
                {
                    s.count_by_prio[p.io.priority]++;
                    s.wait_by_prio[p.io.priority] += p.io.wait;
                }
                s.service_order.push_back(p.io.rid);
            }

            s.avg_wait = (s.total_requests ? (double)s.total_wait / s.total_requests : 0.0);
            s.avg_wait_by_prio.resize(levels, 0.0);
            for (int pr = 0; pr < levels; ++pr)
            {
                s.avg_wait_by_prio[pr] = (s.count_by_prio[pr] ? (double)s.wait_by_prio[pr] / s.count_by_prio[pr] : 0.0);
            }
            return s;
        }

        void Device::print_table(std::ostream &out) const
        {
            out << "Dispositivo: " << name_ << "  NOW=" << now_ << "\n";
            out << "RID\tOwner\tArr\tSvc\tPrio\tStart\tFinish\tWait\n";
            for (const auto &p : completed_)
            {
                out << p.io.rid << "\t"
                    << p.io.owner << "\t"
                    << p.io.arrival << "\t"
                    << p.io.service << "\t"
                    << p.io.priority << "\t"
                    << p.io.start << "\t"
                    << p.io.finish << "\t"
                    << p.io.wait << "\n";
            }

            auto s = stats();
            out << "\nTotal reqs: " << s.total_requests
                << "  Avg wait: " << s.avg_wait
                << "  Utilization: " << s.utilization * 100.0 << "%\n";

            out << "Promedio de espera por prioridad:\n";
            for (int pr = 0; pr < (int)s.avg_wait_by_prio.size(); ++pr)
            {
                out << "  P" << pr << ": " << s.avg_wait_by_prio[pr]
                    << " (n=" << s.count_by_prio[pr] << ")\n";
            }
        }

        void Device::print_queues(std::ostream &out) const
        {
            out << "NOW=" << now_ << "  ";
            out << "RUNNING=" << (idle() ? std::string("-") : pool_[current_].io.rid) << "\n";
            for (int pr = 0; pr < params_.priority_levels; ++pr)
            {
                out << "Q[P" << pr << "]: [";
                const auto &q = queues_[pr];
                for (size_t i = 0; i < q.size(); ++i)
                {
                    if (i)
                        out << ",";
                    out << pool_[q[i]].io.rid;
                }
                out << "]\n";
            }
        }

        void Device::print_timeline(std::ostream &out) const
        {
            out << "Orden de servicio: ";
            for (size_t i = 0; i < service_order_.size(); ++i)
            {
                if (i)
                    out << " -> ";
                out << service_order_[i];
            }
            out << "\n";
        }

        bool Device::export_timeline_csv(const std::string &csv_path) const
        {
            std::ofstream f(csv_path);
            if (!f.is_open())
                return false;
            // Para una salida simple, usamos start/finish de completadas (una por fila)
            f << "rid,owner,arrival,start,finish,service,priority,wait\n";
            for (const auto &p : completed_)
            {
                f << p.io.rid << ","
                  << p.io.owner << ","
                  << p.io.arrival << ","
                  << p.io.start << ","
                  << p.io.finish << ","
                  << p.io.service << ","
                  << p.io.priority << ","
                  << p.io.wait << "\n";
            }
            return true;
        }

    } // namespace io
} // namespace os
