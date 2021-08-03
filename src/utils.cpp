#include "utils.hpp"

std::vector<std::string_view> utils::tokenize(std::string_view view, const std::string_view& delimiters)
{
    std::vector<std::string_view> vector;
    size_t pos = view.find_first_of(delimiters);

    while (pos != std::string_view::npos) {
        vector.push_back(view.substr(0, pos));
        view.remove_prefix(pos + 1);
        pos = view.find_first_of(delimiters);
    }

    if (!view.empty())
        vector.push_back(view);

    return vector;
}
