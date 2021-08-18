/**
 * @brief Test code for YUYV image compression
 *
 */

#include <fmt/format.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <jpeglib.h>
#include <sched.h>
#include <turbojpeg.h>
#include <chrono>
#include <fstream>
#include <numeric>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "libra/util.hpp"

using namespace std;
using namespace std::chrono;
using namespace cv;
using namespace libra::util;

int main(int argc, char* argv[]) {
    cout << Title("YUYV Compress Speed Test") << endl;

    // set CPU affinity
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) < 0) {
        LOG(ERROR) << "set CPU affinity failed";
    }

    // read raw image
    string fileName = "./data/yuyv.bin";  // yuyv image file
    int width = 1280;                     // image width
    int height = 720;                     // image height
    fstream fs(fileName, ios::in | ios::binary);
    if (!fs.is_open()) {
        LOG(ERROR) << fmt::format("cannot open file \"{}\"", fileName);
        return 0;
    }
    vector<unsigned char> raw = vector<unsigned char>(istreambuf_iterator<char>(fs), {});
    fs.close();

    constexpr size_t kRepeatNum{100};  // repeat num
    constexpr size_t kAlgNum{4};       // algorithm num
    vector<string> saveFiles{"./data/OpenCV.png", "./data/OpenCVTurboJpeg.jpg", "./data/jpeg.jpg",
                             "./data/TurboJpeg.jpg"};

    vector<array<double, kAlgNum>> usedTime(kRepeatNum);
    vector<unsigned char> yuvData;
    for (size_t i = 0; i < kRepeatNum; i++) {
        {
            // 1. Convert YUYV to BGR using OpenCV, then write to file
            auto t0 = steady_clock::now();
            Mat yuv(height, width, CV_8UC2, raw.data());
            Mat bgr;
            cvtColor(yuv, bgr, COLOR_YUV2BGR_YUYV);
            imwrite(saveFiles[0], bgr);
            auto t1 = steady_clock::now();
            auto dt = duration_cast<duration<double>>(t1 - t0).count();
            cout << fmt::format("[{}/{}] OpenCV = {:.5f} s", i, kRepeatNum, dt);
            usedTime[i][0] = dt;
        }

        {
            // 2. Convert YUYV to BGR using OpenCV, then compress BGR image using turbo-jpeg, finally write to file
            // init compressor
            tjhandle compressor = tjInitCompress();

            auto t0 = steady_clock::now();
            Mat yuv(height, width, CV_8UC2, raw.data());
            Mat bgr;
            cvtColor(yuv, bgr, COLOR_YUV2BGR_YUYV);

            // compress BGR image using turbojpeg
            unsigned char* dest = nullptr;  // dest buffer
            unsigned long destSize{0};      // dest size
            if (tjCompress2(compressor, bgr.data, width, 0, height, TJPF_BGR, &dest, &destSize, TJSAMP_444, 95,
                            TJFLAG_FASTDCT) != 0) {
                LOG(ERROR) << fmt::format("turbo jpeg compress error: {}", tjGetErrorStr2(compressor));
            }
            // write to file
            fstream fs(saveFiles[1], ios::out | ios::binary);
            if (!fs.is_open()) {
                LOG(ERROR) << fmt::format("cannot create file \"{}\"", saveFiles[1]);
            }
            fs.write(reinterpret_cast<const char*>(dest), destSize);
            fs.close();

            // release turbo jpeg data buffer
            tjFree(dest);

            auto t1 = steady_clock::now();
            auto dt = duration_cast<duration<double>>(t1 - t0).count();
            cout << fmt::format(", OpenCV+TurboJpeg = {:.5f} s", dt);
            usedTime[i][1] = dt;

            // destory compressor
            tjDestroy(compressor);
        }

        {
            // 3. Compress YUYV to JPG using jpeg-lib, then write to file
            auto t0 = steady_clock::now();
            unsigned char* dest = nullptr;  // dest buffer
            unsigned long destSize{0};      // dest size

            jpeg_compress_struct cinfo;
            jpeg_error_mgr jerr;
            JSAMPROW rowPtr[1];
            cinfo.err = jpeg_std_error(&jerr);
            jpeg_create_compress(&cinfo);
            jpeg_mem_dest(&cinfo, &dest, &destSize);
            cinfo.image_width = width & -1;
            cinfo.image_height = height & -1;
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_YCbCr;
            cinfo.dct_method = JDCT_IFAST;

            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, 95, TRUE);
            jpeg_start_compress(&cinfo, TRUE);

            vector<uint8_t> tmprowbuf(width * 3);
            JSAMPROW row_pointer[1];
            row_pointer[0] = &tmprowbuf[0];
            while (cinfo.next_scanline < cinfo.image_height) {
                unsigned i, j;
                unsigned offset = cinfo.next_scanline * cinfo.image_width * 2;  // offset to the correct row
                for (i = 0, j = 0; i < cinfo.image_width * 2; i += 4, j += 6) {
                    // input strides by 4 bytes, output strides by 6 (2 pixels)
                    tmprowbuf[j + 0] = raw.data()[offset + i + 0];  // Y (unique to this pixel)
                    tmprowbuf[j + 1] = raw.data()[offset + i + 1];  // U (shared between pixels)
                    tmprowbuf[j + 2] = raw.data()[offset + i + 3];  // V (shared between pixels)
                    tmprowbuf[j + 3] = raw.data()[offset + i + 2];  // Y (unique to this pixel)
                    tmprowbuf[j + 4] = raw.data()[offset + i + 1];  // U (shared between pixels)
                    tmprowbuf[j + 5] = raw.data()[offset + i + 3];  // V (shared between pixels)
                }
                jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }

            jpeg_finish_compress(&cinfo);

            // write to file
            fstream fs(saveFiles[2], ios::out | ios::binary);
            if (!fs.is_open()) {
                LOG(ERROR) << fmt::format("cannot create file \"{}\"", saveFiles[1]);
            }
            fs.write(reinterpret_cast<const char*>(dest), destSize);
            fs.close();

            // release data
            jpeg_destroy_compress(&cinfo);

            // record time
            auto t1 = steady_clock::now();
            auto dt = duration_cast<duration<double>>(t1 - t0).count();
            cout << fmt::format(", JPEG = {:.5f} s", dt);
            usedTime[i][2] = dt;
        }

        {
            // 4. convert YUYV(YUV422 Packed) to YUV(YUV422 Planar), then compress using TurboJpeg, and finally write to
            // file
            // init compressor
            tjhandle compressor = tjInitCompress();

            auto t0 = steady_clock::now();
            int length = 2 * width * height;
            if (yuvData.size() != length) {
                yuvData.resize(length);
            }
            unsigned char* pY = yuvData.data();
            unsigned char* pU = yuvData.data() + width * height;
            unsigned char* pV = yuvData.data() + width * height * 3 / 2;
            for (int i = 0; i < length; i = i + 4) {
                *pY = raw[i++];
                *pU = raw[i++];
                ++pY;
                ++pU;
                *pY = raw[i++];
                *pV = raw[i++];
                ++pY;
                ++pV;
            }

            // compress
            unsigned char* dest = nullptr;  // dest buffer
            unsigned long destSize{0};      // dest size

            if (tjCompressFromYUV(compressor, yuvData.data(), width, 1, height, TJSAMP_422, &dest, &destSize, 95,
                                  TJFLAG_FASTDCT) != 0) {
                LOG(ERROR) << fmt::format("turbo jpeg compress error: {}", tjGetErrorStr2(compressor));
            }

            // write to file
            fstream fs(saveFiles[3], ios::out | ios::binary);
            if (!fs.is_open()) {
                LOG(ERROR) << fmt::format("cannot create file \"{}\"", saveFiles[1]);
            }
            fs.write(reinterpret_cast<const char*>(dest), destSize);
            fs.close();

            // release turbo jpeg data buffer
            tjFree(dest);

            // record time
            auto t1 = steady_clock::now();
            auto dt = duration_cast<duration<double>>(t1 - t0).count();
            cout << fmt::format(", TurboJpeg = {:.5f} s", dt) << endl;
            usedTime[i][3] = dt;

            // destory compressor
            tjDestroy(compressor);
        }
    }

    // calculate the average time
    array<double, kAlgNum> averageTime;
    for (size_t i = 0; i < kAlgNum; ++i) {
        averageTime[i] = accumulate(usedTime.begin(), usedTime.end(), 0.,
                                    [&](const double& a, const array<double, kAlgNum>& v) { return a + v[i]; }) /
                         kRepeatNum;
    }
    cout << endl
         << fmt::format(
                "Average: OpenCV = {:.5f} s, OpenCV+TurboJpeg = {:.5f} s, JPEG = {:.5f} s, TurboJpeg = {:.5f} s",
                averageTime[0], averageTime[1], averageTime[2], averageTime[3])
         << endl;

    return 0;
}