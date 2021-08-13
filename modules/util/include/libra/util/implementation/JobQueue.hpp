#include <glog/logging.h>

namespace libra {
namespace util {

// Default constructor
template <typename T>
JobQueue<T>::JobQueue() : JobQueue(std::numeric_limits<std::size_t>::max()) {}

// Constructor with maximum job numbers
template <typename T>
JobQueue<T>::JobQueue(const std::size_t& maxJobNums) : maxJobNums_(maxJobNums), stop_(false) {}

// Destructor, stop all job queue and exit
template <typename T>
JobQueue<T>::~JobQueue() {
    stop();
}

// Get the number of pushed but not popped jobs in the queue
template <typename T>
std::size_t JobQueue<T>::size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return jobs_.size();
}

// Push a new job to the queue, waits if the number of jobs is exceeded
template <typename T>
bool JobQueue<T>::push(const T& data) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (jobs_.size() >= maxJobNums_ && !stop_) {
        popCondition_.wait(lock);
    }

    if (stop_) {
        return false;
    } else {
        jobs_.emplace(data);
        pushCondition_.notify_one();
        return true;
    }
}

// Push a new job to the queue with move, waits if the number of jobs is exceeded
template <typename T>
bool JobQueue<T>::push(T&& data) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (jobs_.size() >= maxJobNums_ && !stop_) {
        LOG(WARNING) << "queue is full";
        popCondition_.wait(lock);
    }

    if (stop_) {
        return false;
    } else {
        jobs_.emplace(std::move(data));
        pushCondition_.notify_one();
        return true;
    }
}

// Pop a job from the queue, wait if there is no job in the queue
template <typename T>
typename JobQueue<T>::Job JobQueue<T>::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (jobs_.empty() && !stop_) {
        pushCondition_.wait(lock);
    }

    if (stop_) {
        return Job();
    } else {
        Job job(jobs_.front());
        jobs_.pop();
        popCondition_.notify_one();
        if (jobs_.empty()) {
            emptyCondition_.notify_all();
        }
        return job;
    }
}

// Wait for all jobs to bo popped and then stop the queue
template <typename T>
void JobQueue<T>::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!jobs_.empty()) {
        emptyCondition_.wait(lock);
    }
}

// Stop the queue
template <typename T>
void JobQueue<T>::stop() {
    stop_ = true;
    pushCondition_.notify_all();
    popCondition_.notify_all();
}

// Clear all pushed and not popped jobs from the queue
template <typename T>
void JobQueue<T>::clear() {
    std::unique_lock<std::mutex> lock(mutex_);
    std::queue<T> emptyJobs;
    std::swap(jobs_, emptyJobs);
}

}  // namespace util
}  // namespace libra