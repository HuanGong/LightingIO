#ifndef BASIC_STRING_UTILS_OPT_HPP
#define BASIC_STRING_UTILS_OPT_HPP

#include "config.hpp"
#include "detail.hpp"
#include <string>
#include <locale>
#include <algorithm>

NAMESPACE_BEGIN

template <typename T>
static std::string join(const std::vector<T> &v, const std::string &token) {
  std::ostringstream result;
  for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); i++){
    if (i != v.begin())
      result << token;
    result << *i;
  }

  return result.str();
}

static std::vector<std::string> split(const std::string& text,
                                      const std::string& delims,
                                      bool ignore_empty_result = false) {
    std::vector<std::string> tokens;
    std::size_t start = text.find_first_not_of(delims), end = 0;

    while((end = text.find_first_of(delims, start)) != std::string::npos) {
      if (start != end) {
        tokens.push_back(text.substr(start, end - start));
      } else if (!ignore_empty_result) {
        tokens.push_back("");
      }
      start = text.find_first_not_of(delims, end);
    }
    if(start != std::string::npos)
        tokens.push_back(text.substr(start));

    return std::move(tokens);
}

NAMESPACE_END
#endif
