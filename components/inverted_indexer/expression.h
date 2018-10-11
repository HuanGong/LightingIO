#ifndef _LT_COMPONENT_INVERTED_INDEX_EXPRESSION_H_H
#define _LT_COMPONENT_INVERTED_INDEX_EXPRESSION_H_H

#include <vector>
#include <string>
#include <set>
#include <memory>
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
 *     "field": "ip",
 *     "exclude": true,
 *     "exps" [
 *       "192.168.18.*"
 *     ]
 *    }
 *  ]
 *}
*/

/*
 * document ->  多个term，一个term由一个expression表达
 * 倒排表中每个term会有一个倒排表，
 * 每个倒排表维护着一份field具体值到文档id列表的映射关系
 * filed -> doc_id列表
 * */
class Expression;

typedef std::set<int64_t> IdList;
typedef std::unordered_map<std::string, Expression> ExpConditionMap;

class Expression {
public:
  const std::string& FieldName() const {return term_;}
  bool IsExclude() const {return exclude_;}
  void SetExclude(bool exclude) {exclude_ = exclude;};
  void SetFieldName(const std::string& field) {term_ = field;}
  const std::vector<std::string>& Values() const {return values_;}
  std::vector<std::string>& MutableValues() {return values_;}
  void AppendValue(const std::string& value) {values_.push_back(value);};
private:
  std::string term_; // field name
  bool exclude_ = false;
  std::vector<std::string> values_;
};
void to_json(Json& j, const Expression&);
void from_json(const Json& j, Expression&);

class Document {
public:
  const int64_t& DocumentId() const {return document_id_;}
  const ExpConditionMap& Conditions() const {return conditions_;}
  void SetDocumentId(const int64_t id) {document_id_ = id;}
  void AddExpression(const Expression& exp);
  bool HasConditionExpression(const std::string& field);
private:
  int64_t document_id_ = -1;
  ExpConditionMap conditions_;
};
void to_json(Json& j, const Document&);
void from_json(const Json& j, Document&);

typedef std::shared_ptr<Document> RefDocument;

}
#endif
