#ifndef LT_COMPONENT_SOURCELOADER_READER_H
#define LT_COMPONENT_SOURCELOADER_READER_H

#include <string>
#include <cinttypes>
#include <iostream>
#include <json/json.hpp>

namespace component {
namespace sl {

using Json = nlohmann::json;

class ReaderDelegate {
public:
	virtual void OnFinish(int code) = 0;
	virtual void OnReadData(const std::string& data) = 0;
};



class Loader {
public:
  Loader(ReaderDelegate* watcher, Json reader_config) :
    config_(reader_config),
	  watcher_(watcher) {
  };
  virtual ~Loader() {};

  virtual int Initialize() {return 0;};
  virtual int Load() = 0;
protected:
	Json config_;
	ReaderDelegate* watcher_;
};

}}
#endif
