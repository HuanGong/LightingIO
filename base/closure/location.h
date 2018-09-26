#ifndef BASE_LOCATION_H_H_
#define BASE_LOCATION_H_H_

#include <string>

namespace base {

class Location {
public:
  // Constructor should be called with a long-lived char*, such as __FILE__.
  // It assumes the provided value will persist as a global constant, and it
  // will not make a copy of it.
  Location(const Location& other);
  Location(const char* function_name, const char* file_name, int line_number);

  // Returns true if there is source code location info. If this is false,
  // the Location object only contains a program counter or is
  // default-initialized (the program counter is also null).
  bool has_source_info() const { return function_name_ && file_name_; }

  // Will be nullptr for default initialized Location objects and when source
  // names are disabled.
  const char* function_name() const { return function_name_; }

  // Will be nullptr for default initialized Location objects and when source
  // names are disabled.
  const char* file_name() const { return file_name_; }

  // Will be -1 for default initialized Location objects and when source names
  // are disabled.
  int line_number() const { return line_number_; }

  // Converts to the most user-readable form possible. If function and filename
  // are not available, this will return "pc:<hex address>".
  std::string ToString() const;

  static Location CreateFromHere(const char* function_name, const char* file_name, int line_number);
private:
  int line_number_ = -1;
  const char* file_name_ = nullptr;
  const char* function_name_ = nullptr;
  const void* program_counter_ = nullptr;
};

#define FROM_HERE FROM_HERE_WITH_EXPLICIT_FUNCTION(__FUNCTION__)
#define FROM_HERE_WITH_EXPLICIT_FUNCTION(function_name) base::Location::CreateFromHere(function_name, __FILE__, __LINE__)

} //end namesapce base

#endif
