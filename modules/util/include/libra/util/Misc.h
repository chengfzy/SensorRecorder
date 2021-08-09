#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <numeric>
#include <string>

namespace libra {
namespace util {

/**
 * @brief Create directory if not exist
 * @param dir  Directory path
 */
void createDirIfNotExist(const std::string& dir);

/**
 * @brief Remove all its subdirectories and file recursively in directory
 *
 * @param dir   Directory path
 */
void removeDir(const std::string& dir);

/**
 * @brief Orthogonalize rotation matrix
 * @param R Rotation matrix
 * @return Orthogonalized rotation matrix
 */
inline Eigen::Matrix3d orthogonalize(const Eigen::Matrix3d& R) { return Eigen::AngleAxisd(R).toRotationMatrix(); }

/**
 * @brief Check whether two numbers are close enough within eps
 *
 * @tparam T    Number type
 * @param a     First number
 * @param b     Second number
 * @param eps   Tolerance for check closeness
 * @return  True for two numbers are close, otherwise return false
 */
template <typename T>
inline bool close(const T& a, const T& b, const T& eps = std::numeric_limits<T>::epsilon()) {
    return std::abs(a - b) < eps ? true : false;
}

}  // namespace util
}  // namespace libra
