#ifndef LIGHTINGIO_SOURCE_LOADER_PARSER_H
#define LIGHTINGIO_SOURCE_LOADER_PARSER_H

#include <string>
#include <json/json.hpp>

namespace component {
namespace sl{

using Json = nlohmann::json;

class ParserDelegate {
public:
  virtual void OnContentParsed(const Json& data) = 0;
private:
};

class Parser {
public:
	Parser(ParserDelegate* d, Json& config);
	virtual ~Parser();

  virtual bool Initialize() {return true;};

  virtual bool ParseContent(const std::string& content) = 0; 
protected:
  Json config_; 
  ParserDelegate* delegate_;
};

}}
#endif //LIGHTINGIO_COLUMN_PARSER_H
