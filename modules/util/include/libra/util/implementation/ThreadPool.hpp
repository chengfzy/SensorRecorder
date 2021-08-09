namespace libra {
namespace util {

// Add task to thread pool
template <class F, class... Args>
auto ThreadPool::addTask(F&& f, Args&&... args) -> std::future<std::result_of_t<F(Args...)>> {
    using ReturnType = std::result_of_t<F(Args...)>;
    auto task =
        std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<ReturnType> result = task->get_future();

    // add task to task queue
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stop_) {
            throw std::runtime_error("cannot add task to stopped thread pool");
        }
        tasks_.emplace([task]() { (*task)(); });
    }

    // notify one worker
    taskCondition_.notify_one();

    return result;
}

}  // namespace util
}  // namespace libra