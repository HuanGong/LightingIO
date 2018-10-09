#ifndef LT_COMPONENT_SOURCELOADER_READER_H
#define LT_COMPONENT_SOURCELOADER_READER_H

#include <string>
#include <cinttypes>
#include <iostream>
#include <nlohmann/json.hpp>

namespace component {
namespace sl {

using Json = nlohmann::json;

class LoaderDelegate {
public:
  virtual void OnStart() {};
	virtual void OnFinish(int code) {};
	virtual void OnReadData(const std::string& data) = 0;
};

class Loader {
public:
  Loader(LoaderDelegate* watcher, const Json& reader_config) :
    config_(reader_config),
	  watcher_(watcher) {
  };
  virtual ~Loader() {};

  virtual int Initialize() {return 0;};
  virtual int Load() {return 0;}
  void SetParserContentMode(const std::string& mode) {
  	parser_content_mode_ = mode;
  };
protected:
  Json config_;
	LoaderDelegate* watcher_;
	std::string parser_content_mode_;
};

}}
#endif
