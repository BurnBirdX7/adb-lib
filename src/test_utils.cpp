#include <iostream>
#include "utils.hpp"


int main() {

    std::string to_tokenize = "system_type:serial-serial:prop1;prop2";

    auto vec = utils::tokenize(to_tokenize, ":");

    for (const auto& str : vec)
        std::cout << str << std::endl;

    return 0;
}