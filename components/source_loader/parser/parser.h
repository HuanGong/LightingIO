#ifndef LIGHTINGIO_SOURCE_LOADER_PARSER_H
#define LIGHTINGIO_SOURCE_LOADER_PARSER_H

#include <string>
#include "json/json.hpp"

namespace component {
namespace sl{

using Json = nlohmann::json;

typedef std::function<void (const Json& data)> ParsedContentCallback;

typedef enum {
  ValueTypeInt = 1,
  ValueTypeDouble =2,
  ValueTypeString = 3,
  ValueTypeBoolen = 4,
  ValueTypeJsonObj = 5,
  ValueTypeInvalid
} SourceValueType;

const std::string& ValueTypeToString(SourceValueType t);
SourceValueType ToSourceValueType(const std::string& type);

class ParserDelegate {
public:
	virtual void OnContentParsed(const Json& data) = 0;
};

class Parser {
public:
	virtual ~Parser();

  virtual bool Initialize(const Json& config);
  void SetDelegate(ParserDelegate* d);

  virtual bool ParseContent(const std::string& content) = 0; 

  virtual bool CheckDefault(SourceValueType t, Json& default_value);


  // use for reset status
  virtual void OnSourceLoadFinished(const int code) {};
  const std::string& ParserType() const {return type_;};
  const std::string& PrimaryKey() const {return primary_;};
  const std::string& ContentMode() const {return content_mode_;}

protected:
	std::string type_;
	std::string primary_;
	std::string content_mode_;
	ParserDelegate* delegate_ = nullptr;
};

}}
#endif //LIGHTINGIO_COLUMN_PARSER_H
