#ifndef _LT_COMPONENT_INVERTED_INDEX_EXPRESSION_H_H
#define _LT_COMPONENT_INVERTED_INDEX_EXPRESSION_H_H

#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace component {

using Json = nlohmann::json;

class Expression;

typedef std::set<int64_t> IdList;
typedef std::unordered_map<std::string, Expression> ExpConditionMap;

class JsonObject {
public:
  bool IsValid() const { return valid_; }
  void SetValid(bool v) { valid_ = v; }

private:
  bool valid_ = true;
};

class Expression : public JsonObject {
public:
  const std::string& FieldName() const { return term_; }
  bool IsExclude() const { return exclude_; }
  void SetExclude(bool exclude) { exclude_ = exclude; };
  void SetFieldName(const std::string& field) { term_ = field; }
  const std::vector<std::string>& Values() const { return values_; }
  std::vector<std::string>& MutableValues() { return values_; }
  void AppendValue(const std::string& value) { values_.push_back(value); };

private:
  std::string term_;  // field name
  bool exclude_ = false;
  std::vector<std::string> values_;
};
void to_json(Json& j, const Expression&);
void from_json(const Json& j, Expression&);

class Document : public JsonObject {
public:
  const int64_t& DocumentId() const { return document_id_; }
  const ExpConditionMap& Conditions() const { return conditions_; }
  void SetDocumentId(const int64_t id) { document_id_ = id; }
  void AddExpression(const Expression& exp);
  bool HasConditionExpression(const std::string& field);

private:
  int64_t document_id_ = -1;
  ExpConditionMap conditions_;
};
void to_json(Json& j, const Document&);
void from_json(const Json& j, Document&);

typedef std::shared_ptr<Document> RefDocument;

}  // namespace component
#endif
