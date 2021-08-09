#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace libra {
namespace util {

/**
 * @brief Thread pool with server workers to do a mount of task
 */
class ThreadPool {
  public:
    /**
     * @brief Construct thread pool with thread number
     * @param threadNum Thread number
     */
    explicit ThreadPool(const std::size_t& threadNum);

    /**
     * @brief Destructor, stop all task and exit
     */
    ~ThreadPool();

  public:
    /**
     * @brief Add task to thread pool
     * @tparam F    Task function/class type
     * @tparam Args Task parameters type
     * @param f     Task function
     * @param args  Task function parameters
     * @return Task result after finished
     */
    template <class F, class... Args>
    auto addTask(F&& f, Args&&... args) -> std::future<std::result_of_t<F(Args...)>>;

    /**
     * @brief Stop thread pool
     */
    void stop();

    /**
     * @brief Wait all task finish
     */
    void wait();

  private:
    std::vector<std::thread> workers_;         // work thread
    std::queue<std::function<void()>> tasks_;  // task queue

    bool stop_;                                // stop flag
    int activeWorkerNum_;                      // active worker numbers
    std::mutex mutex_;                         // mutex for synchronization
    std::condition_variable taskCondition_;    // condition variable when new task added
    std::condition_variable finishCondition_;  // condition variable when task is finished
};

}  // namespace util
}  // namespace libra

#include "implementation/ThreadPool.hpp"
