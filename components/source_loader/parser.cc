
#include "parser.h"

namespace component {
namespace sl{

Parser::Parser(ParserDelegate* d, Json& config) 
  : config_(config),
    delegate_(d) {
}
Parser::~Parser() {
};


};
