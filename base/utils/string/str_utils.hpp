#ifndef LIGHTING_BASE_UTILS_StrUtils_H
#define LIGHTING_BASE_UTILS_StrUtils_H

// COPY FROM http://tum-i5.github.io/utils/index.html BSD-3 LICIENCE

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace base {

class StrUtils {
public:
  static bool Replace(std::string &str, const std::string &from,
                      const std::string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) return false;
    str.replace(start_pos, from.length(), to);
    return true;
  }

  static bool ReplaceLast(std::string &str, const std::string &from,
                          const std::string &to) {
    size_t start_pos = str.rfind(from);
    if (start_pos == std::string::npos) return false;
    str.replace(start_pos, from.length(), to);
    return true;
  }

  static bool Contains(const std::string& input, const std::string& test) {
    return std::search(input.begin(), input.end(), test.begin(), test.end()) != input.end();
  }

  static bool StartsWith(const std::string &str, const std::string &prefix) {
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
  }

  static bool EndsWith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
  }

  template <typename T>
  static std::string ToString(T value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
  }

  template<typename T>
  static bool ParseTo(const std::string& str, T& out) {
    std::istringstream iss = std::istringstream(str);
    iss >> out;
    return iss.good();
  }

  template <typename T>
  static T Parse(const std::string &str) {
    T result;
    std::istringstream(str) >> result;
    return result;
  }

  static void ToUpper(char *s) {
    for (size_t i = 0; s[i]; i++) s[i] = toupper(s[i]);
  }

  static void ToUpper(std::string &str) {
    for (size_t i = 0; str[i]; i++) str[i] = toupper(str[i]);
  }

  static void ToLower(char *s) {
    for (size_t i = 0; s[i]; i++) s[i] = tolower(s[i]);
  }

  static void ToLower(std::string &str) {
    for (size_t i = 0; str[i]; i++) str[i] = tolower(str[i]);
  }

  static std::string& TrimLeft(std::string &s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
  }

  static std::string& TrimRight(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace)))
                .base(),
            s.end());
    return s;
  }

  static std::string& Trim(std::string &s) { return TrimLeft(TrimRight(s)); }

  template <typename T>
  static std::string Join(const std::vector<T> &v, const std::string &token) {
    std::ostringstream result;
    for (const auto &i = v.begin(); i != v.end(); i++) {
      if (i != v.begin()) {
        result << token;
      }
      result << *i;
    }
    return result.str();
  }

  template <typename T>
  inline static std::vector<T> ParseArray(const std::string &str, const char delim) {
    std::vector<T> elems;
    std::istringstream f(str);
    std::string s;
    while (std::getline(f, s, delim)) {
      elems.push_back(Parse<T>(s));
    }
    return elems;
  }

  static std::vector<std::string> Split(const std::string &str, const char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(str);

    std::string item;
    while (getline(ss, item, delim)) elems.push_back(item);

    return std::move(elems);
  }

  static std::vector<std::string> Split(const std::string &text,
                                        const std::string &delims,
                                        bool ignore_empty_result = false) {
    std::vector<std::string> tokens;
    std::size_t start = text.find_first_not_of(delims), end = 0;

    while ((end = text.find_first_of(delims, start)) != std::string::npos) {
      if (start != end) {
        tokens.push_back(text.substr(start, end - start));
      } else if (!ignore_empty_result) {
        tokens.push_back("");
      }
      start = text.find_first_not_of(delims, end);
    }
    if (start != std::string::npos) {
      tokens.push_back(text.substr(start));
    }
    return std::move(tokens);
  }
};

template <>
inline std::string StrUtils::Parse(const std::string &str) {
  return str;
}

template <>
inline const char* StrUtils::Parse(const std::string &str) {
  return str.c_str();
}

template <>
inline bool StrUtils::Parse(const std::string &str) {
  std::string s = str;
  ToLower(s);

  if (s == "1",
      s == "on" ||
      s == "yes" ||
      s == "true") {
    return true;
  }
  return Parse<bool>(str);
}

}  // namespace base

#endif
