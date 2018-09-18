//
// Created by gh on 18-9-18.
//

#ifndef LIGHTINGIO_COLUMN_PARSER_H
#define LIGHTINGIO_COLUMN_PARSER_H

#include <string>
#include "components/source_loader/parser.h"

namespace component {
namespace sl{

class ColumnParser : public Parser {
public:
	ColumnParser(ParserDelegate* d, Json& config);
	~ColumnParser();
  
  //override parser
  bool Initialize() override;
  bool ParseContent(const std::string& content) override; 
private:
};

}}
#endif //LIGHTINGIO_COLUMN_PARSER_H
