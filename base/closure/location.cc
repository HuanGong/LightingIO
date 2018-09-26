
#include "location.h"

namespace base {

Location::Location(const Location& other) = default;

Location::Location(const char* function, const char* file, int line)
  : line_number_(line),
    file_name_(file),
    function_name_(function) {
}
std::string Location::ToString() const {
  if (has_source_info()) {
    return std::string(function_name_) + "@" + file_name_ + ":" + std::to_string(line_number_);
  }
  return "";
  //return StringPrintf("pc:%p", program_counter_);
}

#if defined(__clang__)
#define RETURN_ADDRESS() nullptr
#elif defined(__GNUC__) || defined(__GNUG__)
#define RETURN_ADDRESS() __builtin_extract_return_addr(__builtin_return_address(0))
#elif defined(_MSC_VER)
#define RETURN_ADDRESS() _ReturnAddress()
#else
#define RETURN_ADDRESS() nullptr
#endif

Location Location::CreateFromHere(const char* function_name, const char* file_name, int line_number) {
  return Location(function_name, file_name, line_number);
  //return Location(function_name, file_name, line_number, RETURN_ADDRESS());
}

}  // namespace base
