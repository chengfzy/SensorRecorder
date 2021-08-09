/**
 * @brief Some extension to Eigen, such as serialization for json
 */
#pragma once
#include <Eigen/Core>
#include <nlohmann/json.hpp>

namespace libra {
namespace util {

/**
 * @brief Check whether the Vector is fixed size with input dimension
 *
 * @tparam Vector       To be checked Eigen matrix
 * @tparam DimensionNum The dimension number
 */
template <typename Vector, int DimensionNum,
          typename = std::enable_if_t<Vector::RowsAtCompileTime == DimensionNum && Vector::ColsAtCompileTime == 1>>
struct IsFixedSizeVector : std::true_type {};

/**
 * @brief Check whether the Matrix is fixed size with input dimension
 *
 * @tparam Matrix   To be checked Eigen matrix
 * @tparam RowsNum  The dimension number in row
 * @tparam ColsNum  The dimension number in col
 */
template <typename Matrix, int RowsNum, int ColsNum,
          typename = std::enable_if_t<Matrix::RowsAtCompileTime == RowsNum && Matrix::ColsAtCompileTime == ColsNum>>
struct IsFixedSizeMatrix : std::true_type {};

/**
 * @brief Check whether the Matrix is fixed size with input dimension, or with dynamic size
 *
 * @tparam Matrix   To be checked Eigen matrix
 * @tparam RowsNum  The dimension number in row
 * @tparam ColsNum  The dimension number in col
 */
template <typename Matrix, int RowsNum, int ColsNum,
          typename =
              std::enable_if_t<(Matrix::RowsAtCompileTime == RowsNum || Matrix::RowsAtCompileTime == Eigen::Dynamic) &&
                               (Matrix::ColsAtCompileTime == ColsNum || Matrix::ColsAtCompileTime == Eigen::Dynamic)>>
struct IsFixedSizeOrDynamicMatrix : std::true_type {};

/**
 * @brief Check whether the Matrix is fixed rows with input dimension
 *
 * @tparam Matrix   To be checked Eigen matrix
 * @tparam RowsNum  The dimension number in row
 */
template <typename Matrix, int RowsNum, typename = std::enable_if_t<Matrix::RowsAtCompileTime == RowsNum>>
struct IsFixedRowsMatrix : std::true_type {};

/**
 * @brief Check whether the Matrix is fixed cols with input dimension
 *
 * @tparam Matrix   To be checked Eigen matrix
 * @tparam ColsNum  The dimension number in col
 */
template <typename Matrix, int ColsNum, typename = std::enable_if_t<Matrix::ColsAtCompileTime == ColsNum>>
struct IsFixedColsMatrix : std::true_type {};

/**
 * @brief Get the Scalar type for the binary operation for two members, one is type of Scalar type and another is Eigen
 * matrix
 *
 * @tparam Scalar   Scalar type for one member
 * @tparam Derived  Eigen matrix for another member
 * @return The returned binary operation type
 */
template <typename Scalar, typename Derived>
using ReturnScalar = typename Eigen::ScalarBinaryOpTraits<Scalar, typename Derived::Scalar>::ReturnType;

}  // namespace util
}  // namespace libra

namespace Eigen {

/**
 * @brief Convert Eigen Vector to json, just save it as vector<T>
 */
template <typename _Scalar, int _Rows, int _Options, int _MaxRows, int _MaxCols>
void to_json(nlohmann::json& j, const Matrix<_Scalar, _Rows, 1, _Options, _MaxRows, _MaxCols>& m) {
    j = std::vector<_Scalar>(m.data(), m.data() + m.size());
}

/**
 * @brief Convert json to Eigen Vector, just load it as vector<T>
 */
template <typename _Scalar, int _Rows, int _Options, int _MaxRows, int _MaxCols>
void from_json(const nlohmann::json& j, Matrix<_Scalar, _Rows, 1, _Options, _MaxRows, _MaxCols>& m) {
    std::vector<_Scalar> data = j.get<std::vector<_Scalar>>();
    m = Map<Matrix<_Scalar, _Rows, 1, _Options, _MaxRows, _MaxCols>>(data.data());
}

/**
 * @brief Convert Eigen Matrix to json
 */
template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
void to_json(nlohmann::json& j, const Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& m) {
    j = nlohmann::json{
        {"Rows", m.rows()}, {"Cols", m.cols()}, {"Data", std::vector<_Scalar>(m.data(), m.data() + m.size())}};
}

/**
 * @brief Convert json to Eigen Matrix
 */
template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
void from_json(const nlohmann::json& j, Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& m) {
    int rows = j["Rows"].get<int>();
    int cols = j["Cols"].get<int>();
    std::vector<_Scalar> data = j["Data"].get<std::vector<_Scalar>>();
    m = Map<Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>>(data.data(), rows, cols);
}

}  // namespace Eigen