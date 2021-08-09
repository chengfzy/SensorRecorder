#pragma once

namespace libra {
namespace util {

/**
 * @brief Null deleter. Used for smart pointer, it will do nothing for destructor
 */
struct NullDeleter {
    template <typename T>
    void operator()(const T*) {}
};

}  // namespace util
}  // namespace libra