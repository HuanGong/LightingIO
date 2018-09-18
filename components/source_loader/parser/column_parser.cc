//
// Created by gh on 18-9-18.
//

#include "column_parser.h"

namespace component {
namespace sl{


ColumnParser::ColumnParser(ParserDelegate* d, Json& config)
  : Parser(d, config) {
}

ColumnParser::~ColumnParser() {

}

bool ColumnParser::ParseContent(const std::string& content) {
  return true;
}

bool ColumnParser::Initialize() {
  if (!config_.is_object()) {
    return false;
  }
  
  return true;
}


}}//end  namespace
