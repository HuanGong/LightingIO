//
// Created by gh on 18-9-18.
//

#ifndef LIGHTINGIO_COLUMN_PARSER_H
#define LIGHTINGIO_COLUMN_PARSER_H

#include <map>
#include <string>
#include "parser.h"
#include "../json_class_meta.h"

namespace component {
namespace sl{

struct ColumnInfo : public JsonClassMeta {
	std::string name;
  bool allow_null;
  Json default_value;
	SourceValueType type;
};
void to_json(Json& j, const ColumnInfo& info);
void from_json(const Json& j, ColumnInfo& info);

class ColumnParser : public Parser {
public:
	~ColumnParser();

  //override parser
  bool Initialize(const Json& config) override;
  bool ParseContent(const std::string& content) override;
private:
	bool HanleColumn(const ColumnInfo& info, const std::string& data, Json& out);
  std::string delimiter_;
  bool ignore_header_line_;
  std::vector<std::string> header_;
  std::map<std::string, ColumnInfo> columns_;
};

}}
#endif //LIGHTINGIO_COLUMN_PARSER_H
