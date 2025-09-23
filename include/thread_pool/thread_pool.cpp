#include "thread_pool.h"

ThreadPool::ThreadPool(std::size_t numThreads) {
    start(numThreads);
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::start(std::size_t numThreads) {
    for (std::size_t i = 0; i < numThreads; i++) {
        workers.emplace_back([this] {
            while (true) {
                Task task;

                {
                    std::unique_lock<std::mutex> lock{pool_mutex};
                    cv.wait(lock, [this] {
                        return done || !task_queue.empty();
                    });

                    if (done && task_queue.empty()) {
                        break;
                    }

                    task = std::move(task_queue.front());
                    task_queue.pop();
                }

                task();
            }
        });
    }
}

void ThreadPool::stop() noexcept {
    {
        std::unique_lock<std::mutex> lock{pool_mutex};
        done = true;
    }

    cv.notify_all();

    for (auto &thread : workers) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
