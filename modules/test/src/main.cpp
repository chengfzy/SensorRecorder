#include <glog/logging.h>
#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    testing::InitGoogleTest(&argc, argv);
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;

    return RUN_ALL_TESTS();
}