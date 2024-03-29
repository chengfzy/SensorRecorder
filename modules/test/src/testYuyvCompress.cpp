/**
 * @brief Test code for YUYV image compression
 *
 */

#include <fmt/format.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <jpeglib.h>
#include <turbojpeg.h>
#include <chrono>
#include <fstream>
#include <numeric>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;
using namespace std::chrono;
using namespace cv;

class YuyvCompressTest : public testing::Test {
  protected:
    /**
     * @brief Set up
     *
     */
    void SetUp() override;

    /**
     * @brief Compress image using jpeg-lib, which will compress directly in YUYV format
     *
     * @param raw       Raw image buffer
     * @param dest      Dest image pointer
     * @param destSize  Dest image size
     */
    void compressWithJpeg(const vector<unsigned char>& raw, unsigned char* dest, unsigned long& destSize);

  protected:
    int width = 1280;           // image width
    int height = 720;           // image height
    vector<unsigned char> raw;  // raw data for YUYV image
    bool showImage = true;      // whether to show image
};

// set up
void YuyvCompressTest::SetUp() {
    string fileName = "./data/yuyv.bin";  // yuyv image file

    // read raw image
    fstream fs(fileName, ios::in | ios::binary);
    if (!fs.is_open()) {
        LOG(ERROR) << fmt::format("cannot open file \"{}\"", fileName);
        return;
    }

    raw = vector<unsigned char>(istreambuf_iterator<char>(fs), {});
    fs.close();
}

// Compress image using jpeg-lib
void YuyvCompressTest::compressWithJpeg(const vector<unsigned char>& raw, unsigned char* dest,
                                        unsigned long& destSize) {
    // compress image using YUYV
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

    // finish
    jpeg_finish_compress(&cinfo);

    // release data
    jpeg_destroy_compress(&cinfo);
}

// compress using OpenCV
TEST_F(YuyvCompressTest, UsingOpenCV) {
    if (raw.empty()) {
        return;
    }

    // convert using OpenCV and show
    Mat yuv(height, width, CV_8UC2, raw.data());
    Mat bgr;
    cvtColor(yuv, bgr, COLOR_YUV2BGR_YUYV);
    if (showImage) {
        imshow("BGR Image using OpenCV", bgr);
        waitKey(1);
    }
}

// compress using jpeg lib
TEST_F(YuyvCompressTest, UsingJpeg) {
    // compress image using YUYV
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

    // show image
    Mat buf(1, destSize, CV_8UC1, dest);
    Mat bgr = imdecode(buf, IMREAD_UNCHANGED);
    if (showImage) {
        imshow("BGR Image using Jpeg", bgr);
        waitKey(1);
    }
    // release data
    jpeg_destroy_compress(&cinfo);
}

// compress for only left part using jpeg lib
TEST_F(YuyvCompressTest, UsingJpegLeft) {
    // compress image using YUYV
    unsigned char* dest = nullptr;  // dest buffer
    unsigned long destSize{0};      // dest size

    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    JSAMPROW rowPtr[1];
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &dest, &destSize);
    cinfo.image_width = (width / 2) & -1;
    cinfo.image_height = height & -1;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
    cinfo.dct_method = JDCT_IFAST;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 95, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    vector<uint8_t> tmprowbuf(width / 2 * 3);
    JSAMPROW row_pointer[1];
    row_pointer[0] = &tmprowbuf[0];
    while (cinfo.next_scanline < cinfo.image_height) {
        unsigned i, j;
        unsigned offset = cinfo.next_scanline * cinfo.image_width * 2 * 2;  // offset to the correct row
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

    // show image
    Mat buf(1, destSize, CV_8UC1, dest);
    Mat bgr = imdecode(buf, IMREAD_UNCHANGED);
    if (showImage) {
        imshow("Left BGR Image using Jpeg", bgr);
        waitKey(1);
    }

    // release data
    jpeg_destroy_compress(&cinfo);
}

// compress using TurboJpeg
TEST_F(YuyvCompressTest, UsingTurboJpeg) {
    if (raw.empty()) {
        return;
    }

    // convert YUYV(YUV422 Packed) to YUV(YUV422 Planar)
    int length = 2 * width * height;
    vector<unsigned char> yuvData(width * height * 2);
    unsigned char* pY = yuvData.data();
    unsigned char* pU = yuvData.data() + width * height;
    unsigned char* pV = yuvData.data() + width * height * 3 / 2;
    unsigned char* pRaw = raw.data();
    for (int i = 0; i < length / 4; ++i) {
        *pY++ = *(pRaw++);
        *pU++ = *(pRaw++);
        *pY++ = *(pRaw++);
        *pV++ = *(pRaw++);
    }

    // compress
    tjhandle compressor = tjInitCompress();
    unsigned char* dest = nullptr;  // dest buffer
    unsigned long destSize{0};      // dest size

    if (tjCompressFromYUV(compressor, yuvData.data(), width, 1, height, TJSAMP_422, &dest, &destSize, 95,
                          TJFLAG_FASTDCT) != 0) {
        LOG(ERROR) << fmt::format("turbo jpeg compress error: {}", tjGetErrorStr2(compressor));
    }

    // show image
    Mat buf(1, destSize, CV_8UC1, dest);
    Mat bgr = imdecode(buf, IMREAD_UNCHANGED);
    if (showImage) {
        imshow("BGR Image using TurboJpeg", bgr);
        waitKey(0);
    }

    // release turbo jpeg data buffer
    tjFree(dest);
    // destory compressor
    tjDestroy(compressor);
}

// compress for only left part using TurboJpeg
TEST_F(YuyvCompressTest, UsingTurboJpegLeft) {
    if (raw.empty()) {
        return;
    }

    // convert YUYV(YUV422 Packed) to YUV(YUV422 Planar)
    int newWidth = width / 2;
    int length = 2 * newWidth * height;
    vector<unsigned char> yuvData(length);
    unsigned char* pY = yuvData.data();
    unsigned char* pU = yuvData.data() + newWidth * height;
    unsigned char* pV = yuvData.data() + newWidth * height * 3 / 2;
    for (int j = 0; j < height; ++j) {
        unsigned char* pRaw = raw.data() + j * width * 2;
        for (int i = 0; i < width / 4; ++i) {
            *pY++ = *(pRaw++);
            *pU++ = *(pRaw++);
            *pY++ = *(pRaw++);
            *pV++ = *(pRaw++);
        }
    }

    // compress
    tjhandle compressor = tjInitCompress();
    unsigned char* dest = nullptr;  // dest buffer
    unsigned long destSize{0};      // dest size

    if (tjCompressFromYUV(compressor, yuvData.data(), newWidth, 1, height, TJSAMP_422, &dest, &destSize, 95,
                          TJFLAG_FASTDCT) != 0) {
        LOG(ERROR) << fmt::format("turbo jpeg compress error: {}", tjGetErrorStr2(compressor));
    }

    // show image
    Mat buf(1, destSize, CV_8UC1, dest);
    Mat bgr = imdecode(buf, IMREAD_UNCHANGED);
    if (showImage) {
        imshow("Left BGR Image using TurboJpeg", bgr);
        waitKey(0);
    }

    // release turbo jpeg data buffer
    tjFree(dest);
    // destory compressor
    tjDestroy(compressor);
}

/**
 * @brief Speed test
 *
 * 1. Convert YUYV to BGR using OpenCV, then write to file
 * 2. Convert YUYV to BGR using OpenCV, then compress BGR image using turbo-jpeg, finally write to file
 * 3. Compress YUYV to JPG using jpeg-lib, then write to file
 * 4. Convert YUYV(YUV422 Packed) to YUV(YUV422 Planar), then compress using TurboJpeg, and finally write to file
 */
TEST_F(YuyvCompressTest, SpeedTest) {
    constexpr size_t kRepeatNum{100};  // repeat num
    constexpr size_t kAlgNum{4};       // algorithm num
    vector<string> saveFiles{"./data/OpenCV.png", "./data/OpenCVTurboJpeg.jpg", "./data/jpeg.jpg",
                             "./data/TurboJpeg.jpg"};

    vector<array<double, kAlgNum>> usedTime(kRepeatNum);
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
            auto t0 = steady_clock::now();
            Mat yuv(height, width, CV_8UC2, raw.data());
            Mat bgr;
            cvtColor(yuv, bgr, COLOR_YUV2BGR_YUYV);

            // compress BGR image using turbojpeg
            tjhandle compressor = tjInitCompress();
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

            // destory compressor
            tjDestroy(compressor);

            auto t1 = steady_clock::now();
            auto dt = duration_cast<duration<double>>(t1 - t0).count();
            cout << fmt::format(", OpenCV+TurboJpeg = {:.5f} s", dt);
            usedTime[i][1] = dt;
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
            auto t0 = steady_clock::now();
            int length = 2 * width * height;
            vector<unsigned char> yuvData(width * height * 2);
            unsigned char* pY = yuvData.data();
            unsigned char* pU = yuvData.data() + width * height;
            unsigned char* pV = yuvData.data() + width * height * 3 / 2;
            unsigned char* pRaw = raw.data();
            for (int i = 0; i < length; i += 4) {
                *pY++ = *(pRaw++);
                *pU++ = *(pRaw++);
                *pY++ = *(pRaw++);
                *pV++ = *(pRaw++);
            }

            // compress
            tjhandle compressor = tjInitCompress();
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
            // destory compressor
            tjDestroy(compressor);
            // record time
            auto t1 = steady_clock::now();
            auto dt = duration_cast<duration<double>>(t1 - t0).count();
            cout << fmt::format(", TurboJpeg = {:.5f} s", dt) << endl;
            usedTime[i][3] = dt;
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
}
