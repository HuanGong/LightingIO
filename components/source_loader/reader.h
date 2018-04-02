#ifndef LT_COMPONENT_SOURCELOADER_READER_H
#define LT_COMPONENT_SOURCELOADER_READER_H

#include <string>
#include <cinttypes>

namespace cm {
namespace dsl {

class Reader {
public:
  Reader();
  virtual ~Reader();

  void InseartConfig(const std::string name, std::string val);
  void InitReader();

  virtual int Load(std::string uri) = 0;
  virtual bool OnDataRead(const std::string& data) {return false};
  virtual bool OnRawDataRead(const char* start, uint32_t len) {return false};

private:
  bool use_bytes_;
  std::map<std::string, std::string> reader_configs_;
};

}}
#endif
