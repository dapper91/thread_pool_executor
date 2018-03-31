# C++ Thread Pool Executor

Pure C++14 Thread Pool Executor. No external dependencies.

## Dependencies

* C++14



## Usage example

```c++

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

```


## API

### Constructor

Creates a thread pool executor instance and starts `pool_size` threads.

Constructor arguments:

- `pool_size` - the number of threads to keep in the pool, even if they are idle
- `max_pool_size` - the maximum number of threads to allow in the pool
- `keep_alive_time` - when the number of threads is greater than the `pool_size`, this is the maximum time that excess idle threads will wait for new tasks before terminating
- `max_queue_size` - the maximum number of tasks in the executor queue, if the `max_queue_size` is reached `QueueIsFull` exception will be thrown

### submit

Executes the given task sometime in the future.

Method arguments:

- `func` - the task to execute

### is_shutdown

Returns true if this executor has been shut down.

### shutdown

Initiates an orderly shutdown in which previously submitted tasks are executed, but no new tasks will be accepted.

### terminate

Initiates a termination process in which previously submitted tasks waiting in the queue will be destroyed, no new tasks will be accepted.

### is_terminated

Returns true if this executor has been is_terminated.

### wait

Blocks until all tasks have completed execution after a shutdown request, or the timeout occurs, whichever happens first.

Method arguments:

- `timeout` - the maximum time to wait

## License

Public Domain License
