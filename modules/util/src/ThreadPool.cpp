#include "libra/util/ThreadPool.h"
#include <glog/logging.h>

using namespace std;
using namespace libra::util;

// Construct thread pool with thread number
ThreadPool::ThreadPool(const std::size_t& threadNum) : stop_(false), activeWorkerNum_(0) {
    for (size_t i = 0; i < threadNum; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                // take task from task queue
                function<void()> task;
                {
                    unique_lock<mutex> lock(mutex_);
                    taskCondition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    task = move(tasks_.front());
                    tasks_.pop();
                    ++activeWorkerNum_;
                }

                // do task
                task();

                // set active worker number after finished
                {
                    unique_lock<mutex> lock(mutex_);
                    --activeWorkerNum_;
                }

                finishCondition_.notify_all();
            }
        });
    }
}

// Destructor, stop all task and exit
ThreadPool::~ThreadPool() { stop(); }

// Stop thread pool
void ThreadPool::stop() {
    // set stop flag and clear not started task
    {
        unique_lock<mutex> lock(mutex_);
        if (stop_) {
            return;
        }
        stop_ = true;

        queue<function<void()>> emptyTasks;
        swap(tasks_, emptyTasks);
    }

    // notify active tasks
    taskCondition_.notify_all();

    // wait active task finish
    for (auto& w : workers_) {
        w.join();
    }

    finishCondition_.notify_all();
}

// Wait all task finish
void ThreadPool::wait() {
    unique_lock<mutex> lock(mutex_);
    if (!tasks_.empty() || activeWorkerNum_ > 0) {
        finishCondition_.wait(lock, [this] { return tasks_.empty() && activeWorkerNum_ == 0; });
    }
}