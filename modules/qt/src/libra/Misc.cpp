#include "libra/qt/Misc.h"
#include <glog/logging.h>
#include <opencv2/imgproc.hpp>

using namespace cv;

namespace libra {
namespace qt {

// Convert image from cv::Mat to QImage
QImage matToQImage(const Mat& img) {
    switch (img.type()) {
        case CV_8UC1: {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
            QImage image(img.data, img.cols, img.rows, img.step, QImage::Format_Grayscale8);
#else
            // create color table
            static QVector<QRgb> colorTable;
            if (colorTable.empty()) {
                colorTable.resize(256);
                for (int i = 0; i < 256; ++i) {
                    colorTable[i] = qRgb(i, i, i);
                }
            }
            QImage image(img.data, img.cols, img.rows, img.step, QImage::Format_Indexed8);
            image.setColorTable(colorTable);
#endif
            return image;
        }
        case CV_8UC3: {
            QImage image(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);
            return image.rgbSwapped();
        }
        case CV_8UC4: {
            QImage image(img.data, img.cols, img.rows, img.step, QImage::Format_ARGB32);
            return image;
        }
        default:
            LOG(ERROR) << "can only convert type of CV_8UC1, CV_8UC3 and CV_8UC4";
            return QImage();
    }
}

//  Convert image from QImage to cv::Mat
Mat qImageToMat(const QImage& img, bool cloned) {
    Mat mat;
    switch (img.format()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
        case QImage::Format_Grayscale8:
#else
        case QImage::Format_Indexed8:
#endif
            mat = Mat(img.height(), img.width(), CV_8UC1, const_cast<uchar*>(img.bits()),
                      static_cast<size_t>(img.bytesPerLine()));
            break;
        case QImage::Format_RGB888: {
            mat = Mat(img.height(), img.width(), CV_8UC3, const_cast<uchar*>(img.bits()),
                      static_cast<size_t>(img.bytesPerLine()));
            cvtColor(mat, mat, COLOR_RGB2BGR);
            break;
        }
        case QImage::Format_RGB32: {
            QImage converted = img.convertToFormat(QImage::Format_RGB888);
            mat = Mat(converted.height(), converted.width(), CV_8UC3, const_cast<uchar*>(converted.bits()),
                      static_cast<size_t>(converted.bytesPerLine()));
            cvtColor(mat, mat, COLOR_RGB2BGR);
            break;
        }
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
            mat = Mat(img.height(), img.width(), CV_8UC4, const_cast<uchar*>(img.bits()),
                      static_cast<size_t>(img.bytesPerLine()));
        default:
            LOG(ERROR) << "can only convert image type of Format_Grayscale8(Format_Indexed8), Format_RGB888, "
                          "Format_RGB32, Format_ARGB32 and Format_ARGB32_Premultiplied";
            break;
    }
    return cloned ? mat.clone() : mat;
}

}  // namespace qt
}  // namespace libra