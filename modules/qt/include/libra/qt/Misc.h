#pragma once
#include <QImage>
#include <opencv2/core.hpp>

namespace libra {
namespace qt {

/**
 * @brief Convert image from cv::Mat to QImage
 * @param img   Image with cv::Mat format
 * @return Image with QImage format
 */
QImage matToQImage(const ::cv::Mat& img);

/**
 * @brief Convert image from QImage to cv::Mat
 * @param img       Image with QImage format
 * @param cloned    True for return the cloned image, false will use the input image memory
 * @return  Image with cv::Mat format
 */
cv::Mat qImageToMat(const QImage& img, bool cloned = true);

}  // namespace qt
}  // namespace libra
