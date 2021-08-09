#pragma once
#include <Eigen/Core>

namespace libra {
namespace util {

/**
 * @brief Some constants
 */
class Constant {
  public:
    static constexpr double kG{9.81};     //!< gravitational constant
    static constexpr double kEps{1e-10};  //!< the minimum variable
    static const Eigen::Vector3d kGVec;   //!< gravitational vector in ENU frame
};

}  // namespace util
}  // namespace libra