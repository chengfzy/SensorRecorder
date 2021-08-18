/**
 * @brief Test code for YUYV image compression
 *
 */

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <fstream>

using namespace std;
namespace fs = boost::filesystem;

class YuyvCompressTest : public testing::Test {
  protected:
    void SetUp() override {}

    string file = "./data/image01.png";
};