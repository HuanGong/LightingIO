#ifndef _LT_COMPONENT_INVERTED_INDEX_EXPRESSION_H_H
#define _LT_COMPONENT_INVERTED_INDEX_EXPRESSION_H_H

#include <vector>
#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"

namespace component {

using Json = nlohmann::json;

/* document
 *
 * {
 *  "doc": 001,
 *  "conditions": [
 *    {
 *     "field": "name",
 *     "true_exps" [
 *       "huan",
 *       "xiao"
 *     ]
 *    },
 *    {
 *     "field": "ip",
 *     "true_exps" [
 *       "192.168.18.*"
 *     ],
 *     "false_exps": [
 *       "127.0.0.1"
 *     ]
 *    }
 *  ]
 *}
*/

/*
 * document ->  多个term，一个term由一个expression表达
 * */

class Expression {
public:
  const std::string& FieldName() const {return field_;}
  void SetFieldName(const std::string& field) {field_ = field;}
  const std::vector<std::string>& TrueValues() const {return true_values_;}
  std::vector<std::string>& MutableTrueValues() {return true_values_;}
  void AppendTrueValue(const std::string& value) {true_values_.push_back(value);};

  const std::vector<std::string>& FalseValues() const {return false_values_;}
  std::vector<std::string>& MutableFalseValues() {return false_values_;}
  void AppendFalseValue(const std::string& value) {false_values_.push_back(value);};
private:
  std::string field_; // field name
  std::vector<std::string> true_values_;
  std::vector<std::string> false_values_;
};
void to_json(Json& j, const Expression&);
void from_json(const Json& j, Expression&);

class Document {
public:
  const std::string& DocumentId() const {return document_id_;}
  const std::unordered_map<std::string, Expression>& Conditions() const {return conditions_;}
  void SetDocumentId(const char* id) {document_id_ = id;}
  void SetDocumentId(const std::string& id) {document_id_ = id;}
  void AddExpression(const Expression& exp);
private:
  std::string document_id_;
  std::unordered_map<std::string, Expression> conditions_;
};
void to_json(Json& j, const Document&);
void from_json(const Json& j, Document&);


}
#endif
