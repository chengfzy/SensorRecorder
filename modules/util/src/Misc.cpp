#include "libra/util/Misc.h"
#include <fmt/format.h>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

using namespace std;
using namespace Eigen;
using namespace boost::filesystem;

namespace libra {
namespace util {

// Create directory if not exist
void createDirIfNotExist(const string& dir) {
    if (!is_directory(dir)) {
        CHECK(create_directories(path(dir))) << fmt::format("create directory failed \"{}\"", dir);
    }
}

// Remove all its subdirectories and file recursively in directory
void removeDir(const std::string& dir) {
    if (is_directory(dir)) {
        CHECK(remove_all(path(dir))) << fmt::format("remove directory failed \"{}\"", dir);
    }
}

}  // namespace util
}  // namespace libra