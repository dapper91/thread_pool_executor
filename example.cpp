#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>
#include <thread>
#include <atomic>
#include <random>

#include "thread_pool_executor.hpp"

using namespace std::chrono_literals;

std::random_device rand_dev;

thread_local size_t worker_idx = -1; 
std::atomic_size_t workers_cnt;

void task(size_t i)
{
    if (worker_idx == -1) {
        worker_idx = ++workers_cnt;
    }
    std::stringstream message;

    std::uniform_int_distribution<int> dist(0, 10);
    std::this_thread::sleep_for(std::chrono::seconds(dist(rand_dev)));

    message << "[" << "worker-" << std::setfill('0') << std::setw(2) << worker_idx << "]" << "\t" 
            << "task-" << std::setw(2) << i << " has been compleated." << std::endl;
    std::cout << message.str();
}

int main()
{
    std::uniform_int_distribution<int> dist(0, 500);

    size_t pool_size = 4;
    size_t max_pool_size = 16;
    size_t max_queue_size = 64;
    std::chrono::seconds keep_alive_time = 5s;

    ThreadPoolExecutor executor(pool_size, max_pool_size, keep_alive_time, max_queue_size);

    for (size_t i = 0; i < 100; ++i) {
        executor.submit(std::bind(task, i));
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(rand_dev)));
    }

    executor.shutdown();
    executor.wait();

    return 0;
}