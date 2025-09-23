#include "thread_pool.h"

template<class T>
auto ThreadPool::enqueue(T task) -> std::future<decltype(task())> {
    auto wrapper = std::make_shared<std::packaged_task<decltype(task()) ()>>(std::move(task));

    {
        std::unique_lock<std::mutex> lock{pool_mutex};
        task_queue.emplace([=] {
            (*wrapper)();
        });
    }

    cv.notify_one();
    return wrapper->get_future();
}
