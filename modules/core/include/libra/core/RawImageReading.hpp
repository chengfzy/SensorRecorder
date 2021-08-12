#pragma once
#include <fmt/format.h>
#include <utility>

namespace libra {
namespace core {

/**
 * @brief Raw image reading, include a raw image buffer pointer and its corresponding size
 */
class RawImageReading {
  public:
    /**
     * @brief Construct raw image reading with data buffer pointer and its corresponding size
     *
     * @param buffer  Raw image buffer pointer
     * @param size    Buffer size
     */
    explicit RawImageReading(unsigned char* buffer = nullptr, const std::size_t& size = 0)
        : buffer_(buffer), size_(size) {}

    /**
     * @brief Destructor
     */
    ~RawImageReading() = default;

  public:
    // some getter
    inline const unsigned char* buffer() const { return buffer_; }
    inline unsigned char*& buffer() { return buffer_; }
    inline const std::size_t& size() const { return size_; }
    inline std::size_t& size() { return size_; }

    /**
     * @brief Print raw image reading to output stream
     * @param os            Output stream
     * @param imageReading  Raw image reading
     * @return Output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const RawImageReading& imageReading) {
        os << fmt::format("size = {}", imageReading.size());
        return os;
    }

  private:
    unsigned char* buffer_;  // image buffer
    std::size_t size_;       // image buffer size
};

}  // namespace core
}  // namespace libra