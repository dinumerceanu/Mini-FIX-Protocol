#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "../headers.h"

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(std::size_t numThreads);
    ~ThreadPool();

    template<class T>
    auto enqueue(T task) -> std::future<decltype(task())>;

private:
    std::vector<std::thread> workers;
    std::condition_variable cv;
    std::mutex pool_mutex;
    std::queue<Task> task_queue;
    bool done = false;

    void start(std::size_t numThreads);
    void stop() noexcept;
};

#include "thread_pool.tpp"

#endif
