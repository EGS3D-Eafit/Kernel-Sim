#include "process.hpp"
#include <algorithm>

namespace os
{
    namespace cpu
    {

        static const char *state_str(ProcState s)
        {
            switch (s)
            {
            case ProcState::NEW:
                return "NEW";
            case ProcState::READY:
                return "READY";
            case ProcState::RUNNING:
                return "RUNNING";
            case ProcState::BLOCKED:
                return "BLOCKED";
            case ProcState::SUSPENDED:
                return "SUSPENDED";
            case ProcState::TERMINATED:
                return "TERM";
            }
            return "?";
        }

        int ProcessManager::create_process(const std::string &name, int cpu_burst, int arrival_time)
        {
            int pid = next_pid_++;
            PCB pcb;
            pcb.pid = pid;
            pcb.name = name;
            pcb.state = ProcState::NEW;
            pcb.arrival_time = std::max(0, arrival_time);
            pcb.cpu_total = std::max(0, cpu_burst);
            pcb.cpu_remaining = pcb.cpu_total;
            table_[pid] = pcb;
            return pid;
        }

        bool ProcessManager::suspend(int pid)
        {
            PCB *p = find_mut(pid);
            if (!p)
                return false;

            if (p->state == ProcState::TERMINATED)
                return false;

            if (running_.has_value() && running_.value() == pid)
            {
                // desalojar de CPU
                p->state = ProcState::SUSPENDED;
                running_.reset();
                return true;
            }
            // Si estaba en READY, quitar de la cola
            if (p->state == ProcState::READY)
            {
                auto it = std::find(ready_q_.begin(), ready_q_.end(), pid);
                if (it != ready_q_.end())
                    ready_q_.erase(it);
                p->state = ProcState::SUSPENDED;
                return true;
            }
            // Si está NEW/BLOCKED, permitimos suspender administrativamente
            if (p->state == ProcState::NEW || p->state == ProcState::BLOCKED)
            {
                p->state = ProcState::SUSPENDED;
                return true;
            }
            // Si ya está SUSPENDED, nada que hacer
            return p->state == ProcState::SUSPENDED;
        }

        bool ProcessManager::resume(int pid)
        {
            PCB *p = find_mut(pid);
            if (!p)
                return false;
            if (p->state != ProcState::SUSPENDED)
                return false;
            // vuelve a READY (no lo admite si arrival_time > now_; se asume ya admitido antes)
            p->state = ProcState::READY;
            ready_q_.push_back(pid);
            return true;
        }

        bool ProcessManager::terminate(int pid)
        {
            PCB *p = find_mut(pid);
            if (!p)
                return false;
            if (p->state == ProcState::TERMINATED)
                return true;

            if (running_.has_value() && running_.value() == pid)
            {
                p->finish_time = now_;
                p->state = ProcState::TERMINATED;
                running_.reset();
                return true;
            }

            // si estaba en READY, remuévelo
            if (p->state == ProcState::READY)
            {
                auto it = std::find(ready_q_.begin(), ready_q_.end(), pid);
                if (it != ready_q_.end())
                    ready_q_.erase(it);
            }
            p->finish_time = (p->finish_time < 0 ? now_ : p->finish_time);
            p->state = ProcState::TERMINATED;
            return true;
        }

        bool ProcessManager::block_for(int pid, int ticks)
        {
            if (ticks <= 0)
                return false;
            PCB *p = find_mut(pid);
            if (!p)
                return false;
            if (p->state == ProcState::TERMINATED || p->state == ProcState::SUSPENDED)
                return false;

            // Si está RUNNING, se desaloja a BLOCKED
            if (running_.has_value() && running_.value() == pid)
            {
                p->state = ProcState::BLOCKED;
                p->blocked_until = now_ + ticks;
                running_.reset();
                return true;
            }
            // Si está en READY, lo retiramos de la cola y lo bloqueamos
            if (p->state == ProcState::READY)
            {
                auto it = std::find(ready_q_.begin(), ready_q_.end(), pid);
                if (it != ready_q_.end())
                    ready_q_.erase(it);
                p->state = ProcState::BLOCKED;
                p->blocked_until = now_ + ticks;
                return true;
            }
            // Si está NEW, lo permitimos bloquear (se despertará luego)
            if (p->state == ProcState::NEW || p->state == ProcState::BLOCKED)
            {
                p->state = ProcState::BLOCKED;
                p->blocked_until = now_ + ticks;
                return true;
            }
            return false;
        }

        void ProcessManager::run(int ticks)
        {
            if (ticks <= 0)
                return;
            for (int k = 0; k < ticks; ++k)
            {
                admit_new_arrivals();
                wake_blocked();
                dispatch_if_idle();
                run_one_tick();
            }
        }

        std::optional<PCB> ProcessManager::get(int pid) const
        {
            auto it = table_.find(pid);
            if (it == table_.end())
                return std::nullopt;
            return it->second;
        }

        TableSnapshot ProcessManager::snapshot() const
        {
            TableSnapshot s;
            s.now = now_;
            s.pcbs.reserve(table_.size());
            for (auto &kv : table_)
                s.pcbs.push_back(kv.second);
            s.running_pid = running_;
            // ordenar por pid para presentación estable
            std::sort(s.pcbs.begin(), s.pcbs.end(), [](const PCB &a, const PCB &b)
                      { return a.pid < b.pid; });
            return s;
        }

        void ProcessManager::print_table(std::ostream &out) const
        {
            auto s = snapshot();
            out << "NOW=" << s.now << "  RUNNING=" << (s.running_pid ? std::to_string(*s.running_pid) : "-") << "\n";
            out << "pid  name        state      arr  cpuTot  cpuRem  start  finish  wait  run\n";
            for (const auto &p : s.pcbs)
            {
                out << std::setw(3) << p.pid << "  "
                    << std::setw(10) << p.name << "  "
                    << std::setw(9) << state_str(p.state) << "  "
                    << std::setw(3) << p.arrival_time << "  "
                    << std::setw(6) << p.cpu_total << "  "
                    << std::setw(6) << p.cpu_remaining << "  "
                    << std::setw(5) << p.start_time << "  "
                    << std::setw(6) << p.finish_time << "  "
                    << std::setw(4) << p.waiting_time << "  "
                    << std::setw(3) << p.running_time << "\n";
            }
        }

        void ProcessManager::print_queues(std::ostream &out) const
        {
            out << "READY: [";
            for (size_t i = 0; i < ready_q_.size(); ++i)
            {
                if (i)
                    out << ",";
                out << ready_q_[i];
            }
            out << "]\n";
        }

        PCB *ProcessManager::find_mut(int pid)
        {
            auto it = table_.find(pid);
            if (it == table_.end())
                return nullptr;
            return &it->second;
        }
        const PCB *ProcessManager::find(int pid) const
        {
            auto it = table_.find(pid);
            if (it == table_.end())
                return nullptr;
            return &it->second;
        }

        void ProcessManager::admit_new_arrivals()
        {
            // NEW -> READY cuando llega su tiempo (y no estén suspendidos)
            for (auto &kv : table_)
            {
                PCB &p = kv.second;
                if (p.state == ProcState::NEW && p.arrival_time <= now_)
                {
                    p.state = ProcState::READY;
                    ready_q_.push_back(p.pid);
                }
            }
        }

        void ProcessManager::wake_blocked()
        {
            for (auto &kv : table_)
            {
                PCB &p = kv.second;
                if (p.state == ProcState::BLOCKED && p.blocked_until <= now_)
                {
                    p.blocked_until = -1;
                    p.state = ProcState::READY;
                    ready_q_.push_back(p.pid);
                }
            }
        }

        void ProcessManager::dispatch_if_idle()
        {
            if (running_.has_value())
                return;
            // elegir siguiente READY (FCFS)
            if (!ready_q_.empty())
            {
                int pid = ready_q_.front();
                ready_q_.pop_front();
                PCB *p = find_mut(pid);
                if (p)
                {
                    p->state = ProcState::RUNNING;
                    if (p->start_time < 0)
                        p->start_time = now_;
                    running_ = pid;
                }
            }
        }

        void ProcessManager::run_one_tick()
        {
            // Incrementar contadores
            for (auto &kv : table_)
            {
                PCB &p = kv.second;
                switch (p.state)
                {
                case ProcState::READY:
                    p.waiting_time++;
                    break;
                case ProcState::RUNNING: /* handled below */
                    break;
                default:
                    break;
                }
            }

            if (running_)
            {
                PCB *p = find_mut(*running_);
                if (p)
                {
                    p->cpu_remaining = std::max(0, p->cpu_remaining - 1);
                    p->running_time++;
                    if (p->cpu_remaining == 0)
                    {
                        p->finish_time = now_ + 1; // termina al final de este tick
                        p->state = ProcState::TERMINATED;
                        running_.reset();
                    }
                }
            }

            // avanza reloj al final del tick
            now_ += 1;
        }

        void ProcessManager::deschedule_running_to_ready()
        {
            if (!running_)
                return;
            PCB *p = find_mut(*running_);
            if (!p)
            {
                running_.reset();
                return;
            }
            p->state = ProcState::READY;
            ready_q_.push_back(*running_);
            running_.reset();
        }

    }
}