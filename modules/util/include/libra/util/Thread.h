#pragma once
#include <functional>
#include <list>
#include <mutex>
#include <numeric>
#include <thread>
#include <unordered_map>

namespace libra {
namespace util {

/**
 * @brief Define thread class with control
 */
class Thread {
  public:
    /**
     * @brief Callback function ID, which would used to map the callback function.
     *
     * Here I don't use enum class due to it don't support inherit, and the first value was set to INT_MIN.
     */
    enum {
        CallBackStarted = std::numeric_limits<int>::min(),  //!< callback ID after thread is started
        CallBackFinished,                                   //!< callback ID after thread is finished
    };

    /**
     * @brief Default constructor
     */
    explicit Thread();

    /**
     * @brief Destructor
     */
    virtual ~Thread();

  public:
    /**
     * @brief Check if the thread is started
     * @return True if current thread is started, otherwise return false
     */
    bool isStart() const;

    /**
     * @brief Check if the thread is stopped
     * @return True if current thread is stopped, otherwise return false
     */
    bool isStop() const;

    /**
     * @brief Check if the thread is finished
     * @return True if current thread is finished, otherwise return false
     */
    bool isFinish() const;

    /**
     * @brief Get the unique identifier of the current thread
     * @return  The unique identifier of the current thread
     */
    std::thread::id threadId() const;

    /**
     * @brief Start thread
     */
    virtual void start();

    /**
     * @brief Stop thread
     */
    virtual void stop();

    /**
     * @brief Wait the thread until it's finished
     */
    virtual void wait();

    /**
     * @brief Add callback function that could be triggered in the main run() function
     * @param id    Callback ID
     * @param func  Callback function
     */
    void addCallback(int id, const std::function<void()>& func);

  protected:
    /**
     * @brief Register a new callback. Only the registered callbacks could be set/reset and called from within the
     * thread, hence, this method should be called from the derived thread constructor
     * @param id    Callback ID
     */
    void registerCallback(int id);

    /**
     * @brief Callback to the function with specified ID. If it doesn't exist, it will do nothing
     * @param id    Callback ID
     */
    void callBack(int id) const;

    /**
     * @brief the main run function should be implemented in the child class
     */
    virtual void run() = 0;

    /**
     * @brief The wrapper function which include the start, run() method and finish callback
     */
    void runFunc();

  private:
    std::thread thread_;        // thread to run the detail function
    mutable std::mutex mutex_;  // mutex to control some variable modification
    bool start_;                // flag to indict thread is started
    bool stop_;                 // flag to indict thread is stopped
    bool finish_;               // flag to indict thread is finished
    std::unordered_map<int, std::list<std::function<void()>>> callbacks_;  // callback functions
};

}  // namespace util
}  // namespace libra
