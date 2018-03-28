# C++ Thread Pool Executor

Pure C++14 Thread Pool Executor. No external dependencies.

## Dependencies

* C++14



## Usage example

```c++

#include <iostream>
#include <functional>
#include <random>

#include "thread_pool_executor.hpp"

using namespace std::chrono_literals;

std::random_device rand_dev;


void func(size_t i)
{
    std::uniform_int_distribution<int> dist(0, 10);
    std::this_thread::sleep_for(std::chrono::seconds(dist(rand_dev)));

    std::cout << "hello from " << i << std::endl;
}

int main()
{
    std::uniform_int_distribution<int> dist(0, 500);

    size_t pool_size = 4;
    size_t max_pool_size = 16;    
    size_t max_queue_size = 64;
    std::chrono::seconds keep_alive_time = 5s;

    ThreadPoolExecutor executor(pool_size, max_pool_size, keep_alive_time, max_queue_size);

    for (size_t i = 0; i < 200; ++i) {
        executor.submit(std::bind(func, i));
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(rand_dev)));
    }

    executor.wait(10s);
    executor.shutdown();
    executor.wait();

    return 0;
}

```


## API

### Constructor

Creates a thread pool executor instance and starts `pool_size` threads:

```c++

ThreadPoolExecutor executor(pool_size, max_pool_size, keep_alive_time, max_queue_size);

```

Constructor arguments:

- `pool_size` - the number of threads to keep in the pool, even if they are idle
- `max_pool_size` - the maximum number of threads to allow in the pool
- `keep_alive_time` - when the number of threads is greater than the `pool_size`, this is the maximum time that excess idle threads will wait for new tasks before terminating
- `max_queue_size` - the maximum number of tasks in the executor queue, if the `max_queue_size` is reached `QueueIsFull` exception will be thrown

## License

WTFPL
