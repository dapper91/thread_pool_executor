#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <limits>
#include <type_traits>


namespace concurrent {


class TimeoutError: public std::runtime_error {
public:
    typedef std::runtime_error super;

    TimeoutError(const std::string& what_arg):
        super(what_arg)
    {}
};


class QueueIsClosed: public std::runtime_error {
public:
    typedef std::runtime_error super;

    QueueIsClosed(const std::string& what_arg):
        super(what_arg)
    {}
};


class QueueIsFull: public std::runtime_error {
public:
    typedef std::runtime_error super;

    QueueIsFull(const std::string& what_arg):
        super(what_arg)
    {}
};


class QueueIsEmpty: public std::runtime_error {
public:
    typedef std::runtime_error super;

    QueueIsEmpty(const std::string& what_arg):
        super(what_arg)
    {}
};


template <typename Type, typename Queue = std::queue<Type>>
class ConcurrentQueue {
public:
    typedef typename Queue::container_type      container_type;
    typedef typename Queue::value_type          value_type;
    typedef typename Queue::size_type           size_type;
    typedef typename Queue::reference           reference;
    typedef typename Queue::const_reference     const_reference;

    ConcurrentQueue(size_type max_size = std::numeric_limits<size_t>::max()):
        max_size(max_size), is_shutdown_flag(false)
    {
        if (max_size == 0) {
            throw std::invalid_argument("queue max size is 0");
        }
    }

    ConcurrentQueue(const ConcurrentQueue& other)
    {
        std::lock_guard<std::mutex> lock(other.mx);

        queue = other.queue;
        max_size = other.max_size;
        is_shutdown_flag = other.is_shutdown_flag;
        is_terminated_flag = other.is_terminated_flag;
    }

    ConcurrentQueue(ConcurrentQueue&& other)
    {
        std::lock_guard<std::mutex> lock(other.mx);

        queue = std::move(other.queue);
        max_size = other.max_size;
        is_shutdown_flag = other.is_shutdown_flag;
        is_terminated_flag = other.is_terminated_flag;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mx);
        return queue.empty();
    }

    size_type size() const
    {
        std::lock_guard<std::mutex> lock(mx);
        return queue.size();
    }

    void push(Type&& value)
    {
        std::lock_guard<std::mutex> lock(mx);
        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is closed");
        }
        if (full()) {
            throw QueueIsFull("queue is full");
        }

        queue.push(std::forward<Type>(value));
        cv.notify_one();
    }

    template <typename Duration>
    void push_for(Type&& value, Duration timeout)
    {
        std::unique_lock<std::mutex> lock(mx);

        cv.wait_for(lock, timeout, [this] {
            return !full() || is_shutdown_flag;
        });

        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is shutdown");
        }
        if (full()) {
            throw TimeoutError("queue timeout has been expired");
        }

        queue.push(std::forward<Type>(value));
        cv.notify_one();
    }

    void push(const Type& value)
    {
        std::lock_guard<std::mutex> lock(mx);
        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is shutdown");
        }
        if (full()) {
            throw QueueIsFull("queue is full");
        }

        queue.push(value);
        cv.notify_one();
    }

    template <typename Duration>
    void push_for(const Type& value, Duration timeout)
    {
        std::unique_lock<std::mutex> lock(mx);

        cv.wait_for(lock, timeout, [this] {
            return !full() || is_shutdown_flag;
        });

        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is shutdown");
        }
        if (full()) {
            throw TimeoutError("queue timeout has been expired");
        }

        queue.push(value);
        cv.notify_one();
    }

    template<typename... Args>
    void emplace(Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mx);
        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is shutdown");
        }
        if (full()) {
            throw QueueIsFull("queue is full");
        }

        queue.emplace(std::forward<Args>(args)...);
        cv.notify_one();
    }

    template<typename Duration, typename... Args>
    void emplace_for(Duration timeout, Args&&... args)
    {
        std::unique_lock<std::mutex> lock(mx);

        cv.wait_for(lock, timeout, [this] {
            return !full() || is_shutdown_flag;
        });

        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is shutdown");
        }
        if (full()) {
            throw TimeoutError("queue timeout has been expired");
        }

        queue.emplace(std::forward<Args>(args)...);
        cv.notify_one();
    }

    void pull(Type& value)
    {
        std::lock_guard<std::mutex> lock(mx);
        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is shutdown");
        }
        if (queue.empty()) {
            throw QueueIsEmpty("queue is empty");
        }

        value = std::move(queue.front()); queue.pop();
    }

    template <typename Duration>
    void pull_for(Type& value, Duration timeout)
    {
        std::unique_lock<std::mutex> lock(mx);

        cv.wait_for(lock, timeout, [this] {
            return !queue.empty() || is_shutdown_flag;
        });

        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is shutdown");
        }
        if (queue.empty()) {
            throw TimeoutError("queue timeout has been expired");
        }

        value = std::move(queue.front()); queue.pop();
    }

    Type pull()
    {
        static_assert(std::is_nothrow_move_constructible<Type>::value, "Type pull() method requires noexcept move-constructible type");

        std::lock_guard<std::mutex> lock(mx);
        if (is_shutdown_flag) {
            throw QueueIsClosed("queue is shutdown");
        }
        if (queue.empty()) {
            throw QueueIsEmpty("queue is empty");
        }

        Type value = std::move(queue.front()); queue.pop();

        return std::move(value);
    }

    template <typename Duration>
    Type pull_for(Duration timeout)
    {
        static_assert(std::is_nothrow_move_constructible<Type>::value, "Type pull_for(Duration timeout) method requires noexcept move-constructible type");

        std::unique_lock<std::mutex> lock(mx);

        cv.wait_for(lock, timeout, [this] {
            return !queue.empty() || is_terminated_flag;
        });

        if (is_terminated_flag) {
            throw QueueIsClosed("queue is closed");
        }
        if (queue.empty()) {
            throw TimeoutError("queue timeout has been expired");
        }

        Type value = std::move(queue.front()); queue.pop();

        return std::move(value);
    }

    void shutdown()
    {
        std::lock_guard<std::mutex> lock(mx);

        is_shutdown_flag = true;
        cv.notify_all();
    }

    void terminate()
    {
        std::lock_guard<std::mutex> lock(mx);

        is_shutdown_flag = true;
        is_terminated_flag = true;
        cv.notify_all();
    }

    bool is_shutdown() const
    {
        return is_shutdown_flag;
    }

    bool is_terminated() const
    {
        return is_terminated_flag;
    }

private:
    Queue queue;
    mutable std::mutex mx;

    std::condition_variable cv;
    std::atomic_bool is_shutdown_flag;
    std::atomic_bool is_terminated_flag;

    size_type max_size;

    bool full() const
    {
        return queue.size() >= max_size;
    }
};


} // namespace concurrent