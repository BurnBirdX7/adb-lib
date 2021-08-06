#ifndef ADB_LIB_UTILS_HPP
#define ADB_LIB_UTILS_HPP

#include <vector>
#include <string>
#include <string_view>


namespace utils {

    std::vector<std::string_view> tokenize(std::string_view view, const std::string_view& delimiters);

    std::string dataToHex(const std::string_view& payload);

}

#endif //ADB_LIB_UTILS_HPP
