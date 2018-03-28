#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <map>
#include <functional>
#include <utility>
#include <limits>
#include <chrono>

#include "concurrent_queue.hpp"


using namespace std::chrono_literals;


class ThreadPoolExecutor {
public:
    ThreadPoolExecutor(size_t pool_size, size_t max_pool_size = 0, std::chrono::milliseconds keep_alive_time = 0s, size_t max_queue_size = std::numeric_limits<size_t>::max()):
        pool_size(pool_size),
        max_pool_size(max_pool_size ? max_pool_size : pool_size),
        keep_alive_time(keep_alive_time),
        max_queue_size(max_queue_size),
        queue(max_queue_size)
    {
        for (size_t i = 0; i < pool_size; ++i) {
            std::thread thread(&ThreadPoolExecutor::poll_queue, this);
            workers_map.emplace(thread.get_id(), std::move(thread));
        }
    }

    virtual ~ThreadPoolExecutor()
    {
        if (is_active()) {
            shutdown();
        }

        wait();
    }
    
    template<typename F> 
    void submit(F func)
    {
        queue.emplace(func);

        std::this_thread::yield();
        if (!queue.empty()) {
            std::lock_guard<std::mutex> lock(workers_map_mx);

            if (workers_map.size() < max_pool_size) {
                std::thread thread(&ThreadPoolExecutor::poll_queue, this);
                workers_map.emplace(thread.get_id(), std::move(thread));
            }
        }
    }

    bool is_active()
    {
        return !queue.is_closed();
    }

    void shutdown()
    {
        queue.close();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(workers_map_mx);
        workers_map_cv.wait(lock, [this] {
            return workers_map.empty();
        });
    }

    template <typename Durataion>
    bool wait(Durataion timeout)
    {
        std::unique_lock<std::mutex> lock(workers_map_mx);
        return workers_map_cv.wait_for(lock, timeout, [this] {
            return workers_map.empty();
        });
    }

private:
    size_t pool_size;
    size_t max_pool_size;
    size_t max_queue_size;
    std::chrono::milliseconds keep_alive_time;

    std::mutex workers_map_mx;
    std::condition_variable workers_map_cv;
    std::map<std::thread::id, std::thread> workers_map;

    concurrent::ConcurrentQueue<std::function<void()>> queue;

    void poll_queue()
    {
        std::unique_lock<std::mutex> lock(workers_map_mx, std::defer_lock);
        std::function<void()> func;

        while (true) {
            try {
                queue.pull_for(func, keep_alive_time);
                func();
            }
            catch (concurrent::TimeoutError& ex) {
                lock.lock();
                if (workers_map.size() > pool_size) {
                    break;
                }
                else {
                    lock.unlock();
                }
            }
            catch (concurrent::QueueIsClosed& ex) {
                break;
            }
        }

        on_worker_exit(lock);
    }

    void on_worker_exit(std::unique_lock<std::mutex>& lock)
    {
        if (!lock) {
            lock.lock();
        }

        std::thread::id current_worker_id = std::this_thread::get_id();

        workers_map.at(current_worker_id).detach();
        workers_map.erase(current_worker_id);

        std::notify_all_at_thread_exit(workers_map_cv, std::move(lock));
    }
};
