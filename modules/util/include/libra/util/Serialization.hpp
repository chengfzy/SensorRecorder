/**
 * @brief Serialize(save & load) class from/to json file
 */
#pragma once
#include <fmt/format.h>
#include <glog/logging.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace libra {
namespace util {

/**
 * @brief Save object to json file
 * @tparam T        Object class
 * @param obj       Object
 * @param file      File name
 * @param tagName   Tag name for the object, tag name will emit if input empty
 */
template <typename T>
void saveJson(const T& obj, const std::string& file, const std::string& tagName = "") {
    std::ofstream fs(file);
    CHECK(fs.is_open()) << fmt::format("cannot create file \"{}\"", file);
    nlohmann::json j;
    if (tagName.empty()) {
        j = obj;
    } else {
        j[tagName] = obj;
    }
    fs << j.dump(2);
    fs.close();
}

/**
 * @brief Load object from json file
 * @tparam T        Object type
 * @param file      File name
 * @param tagName   Tag name for the object, tag name will emit if input empty
 * @return Object
 */
template <typename T>
T loadJson(const std::string& file, const std::string& tagName = "") {
    std::ifstream fs(file);
    CHECK(fs.is_open()) << fmt::format("cannot open file \"{}\"", file);
    nlohmann::json j;
    fs >> j;
    fs.close();
    if (tagName.empty()) {
        return j.get<T>();
    } else {
        return j[tagName].get<T>();
    }
}

}  // namespace util
}  // namespace libra