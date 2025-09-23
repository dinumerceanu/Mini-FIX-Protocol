#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <iostream>
#include <future>

class ThreadPool
{
public:
    using Task = std::function<void()>;

    explicit ThreadPool(std::size_t numThreads) {
        start(numThreads);
    }

    ~ThreadPool() {
        stop();
    }

    template<class T>
    auto enqueue(T task) -> std::future<decltype(task())> {
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

private:
    std::vector<std::thread> workers;

    std::condition_variable cv;

    std::mutex pool_mutex;

    std::queue<Task> task_queue;

    bool done = false;

    void start(std::size_t numThreads) {
        for (int i = 0; i < numThreads; i++) {
            workers.emplace_back([=] {
                while (true) {
                    Task task;

                    {
                        std::unique_lock<std::mutex> lock{pool_mutex};

                        cv.wait(lock, [=] {
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

    void stop() noexcept {
        {
            std::unique_lock<std::mutex> lock{pool_mutex};
            done = true;
        }

        cv.notify_all();

        for (auto &thread : workers) {
            thread.join();
        }
    }
};

char func1() {
    return 'a';
}

void myFunc(int i) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    printf("Job %d done\n", i);
}

int main() {
    ThreadPool pool{3};

    {
        // auto f1 = pool.enqueue([] {
        //     return func1();
        // });

        // auto f2 = pool.enqueue([] {
        //     return 2;
        // });

        // std::cout << (f1.get() + f2.get()) << std::endl;
        // std::cout << f1.get() << f2.get() << std::endl;
        for (int i = 0; i < 5; i++) {
            printf("Job %d waiting...\n", i);
            pool.enqueue([=] {
                // printf("Job %d waiting...\n", i);
                myFunc(i);
            });
        }
    }

    return 0;
}
