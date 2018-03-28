# C++ Thread Pool Executor

Pure C++11 Thread Pool Executor. No external dependencies.

## Dependencies

* C++11



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
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(rd)));
    }

    executor.wait(10s);
    executor.shutdown();
    executor.wait();

    return 0;
}

```


## API

### Constructor

Creates a client instance and authenticates on the service:

```c++

client = YaMusicClient('dima', 'password123')

```

Constructor arguments:

- `login` - yandex account user name
- `password` - yandex account password
- `logger` - a python logger to use to instead of a default logger (default: None)
- `remember_me` - force the service to remember the identity of the user between sessions (default: True)

## License

WTFPL
