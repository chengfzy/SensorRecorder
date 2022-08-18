#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace libra {
namespace util {

/**
 * @brief Job queue for the producer-consumer paradigm
 * @tparam T
 */
template <typename T>
class JobQueue {
  public:
    /**
     * @brief Job data
     */
    class Job {
      public:
        Job() : valid_(false) {}
        explicit Job(const T& data) : data_(data), valid_(true) {}
        explicit Job(T&& data) : data_(data), valid_(true) {}

      public:
        // check whether the data is valid
        bool isValid() const { return valid_; }

        // get reference to the data
        T& data() { return data_; }
        const T& data() const { return data_; }

      private:
        T data_;      // job data
        bool valid_;  // flag to indict whether the job data is valid
    };

  public:
    /**
     * @brief Default constructor
     */
    JobQueue();

    /**
     * @brief Constructor with maximum job numbers
     * @param maxJobNums Maximum job numbers
     */
    explicit JobQueue(const std::size_t& maxJobNums);

    /**
     * @brief Destructor, stop all job queue and exit
     */
    ~JobQueue();

    /**
     * @brief Get the number of pushed but not popped jobs in the queue
     * @return The number of pushed but not popped jobs in the queue
     */
    std::size_t size() const;

    /**
     * @brief Return whether the job queue is stopped
     * @return True if the job queue is stopped, otherwise return false
     */
    inline bool isStop() const { return stop_; }

    /**
     * @brief Return whether to drop job data when queue is full
     *
     * @return  True if drop job data when queue if full, otherwise return false
     */
    inline bool dropJob() const { return dropJob_; }

    /**
     * @brief Enable/disable drop job data when queue if full
     *
     * @param drop  True for enable, false for disable
     */
    void enableDropJob(bool enable = true);

    /**
     * @brief Push a new job to the queue, waits if the number of jobs is exceeded
     * @param data New job data
     * @return True if push success, false for push fail
     */
    bool push(const T& data);

    /**
     * @brief Push a new job to the queue with move, waits if the number of jobs is exceeded
     * @param data New job data
     * @return True if push success, false for push fail
     */
    bool push(T&& data);

    /**
     * @brief Pop a job from the queue, wait if there is no job in the queue
     * @return Job popped from the queue
     */
    Job pop();

    /**
     * @brief Wait for all jobs to bo popped and then stop the queue
     */
    void wait();

    /**
     * @brief Stop the queue
     */
    void stop();

    /**
     * @brief Clear all pushed and not popped jobs from the queue
     */
    void clear();

  private:
    std::size_t maxJobNums_;                  // maximum job numbers
    bool dropJob_;                            // drop last job data if queue is full
    std::atomic<bool> stop_;                  // flag to indict whether to stop queue
    std::queue<T> jobs_;                      // job queue
    mutable std::mutex mutex_;                // mutex
    std::condition_variable pushCondition_;   // condition variable after push a jpb
    std::condition_variable popCondition_;    // condition variable after pop a job
    std::condition_variable emptyCondition_;  // condition variable when jos queue is empty
};

}  // namespace util
}  // namespace libra

#include "implementation/JobQueue.hpp"
