#include "libra/util/Thread.h"
#include <fmt/format.h>
#include <glog/logging.h>

using namespace std;
using namespace libra::util;

// Default constructor
Thread::Thread() : start_(false), stop_(false), finish_(false) {
    registerCallback(CallBackStarted);
    registerCallback(CallBackFinished);
}

// Destructor
Thread::~Thread() {
    // wait process finish
    if (isStart()) {
        stop();
        wait();
    }
}

// Check if the thread is started
bool Thread::isStart() const {
    unique_lock<mutex> lock(mutex_);
    return start_;
}

// Check if the thread is started
bool Thread::isStop() const {
    unique_lock<mutex> lock(mutex_);
    return stop_;
}

// Check if the thread is finished
bool Thread::isFinish() const {
    unique_lock<mutex> lock(mutex_);
    return finish_;
}

// Get the unique identifier of the current thread
std::thread::id Thread::threadId() const { return std::this_thread::get_id(); }

// Start thread
void Thread::start() {
    unique_lock<mutex> lock(mutex_);
    CHECK(!start_ || finish_);
    thread_ = thread(&Thread::runFunc, this);
    start_ = true;
    stop_ = false;
    finish_ = false;
}

// Stop thread
void Thread::stop() {
    unique_lock<mutex> lock(mutex_);
    stop_ = true;
}

// Wait the thread until it's finished
void Thread::wait() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

// Add callback function that could be triggered in the main run() function
void Thread::addCallback(int id, const std::function<void()>& func) {
    CHECK(func);
    CHECK_GT(callbacks_.count(id), 0) << fmt::format("Callback ID = {} not registered", id);
    callbacks_[id].emplace_back(func);
}

// Register a new callback
void Thread::registerCallback(int id) { callbacks_.emplace(id, list<function<void()>>()); }

// Callback to the function with specified ID. If it doesn't exist, it will do nothing
void Thread::callBack(int id) const {
    CHECK_GT(callbacks_.count(id), 0) << fmt::format("Callback ID = {} not registered", id);
    for (auto& f : callbacks_.at(id)) {
        f();
    }
}

// The wrapper function which include the start, run() method and finish callback
void Thread::runFunc() {
    callBack(CallBackStarted);
    run();
    {
        unique_lock<mutex> lock(mutex_);
        finish_ = true;
    }
    callBack(CallBackFinished);
}
