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

typedef int64_t DocIdType;
typedef std::set<DocIdType> IdList;

typedef std::unordered_map<std::string, Expression> ExpConditionMap;

class JsonObject {
public:
  void SetValid(bool v) { valid_ = v;}
  bool IsValid() const { return valid_;}
private:
  bool valid_ = true;
};

class Expression : public JsonObject {
public:
  bool IsExclude() const { return exclude_; }
  void SetExclude(bool exclude) { exclude_ = exclude;};
  const std::string& FieldName() const { return term_;}
  void SetFieldName(const std::string& field) {term_ = field;}
  std::vector<std::string>& MutableValues() { return values_;}
  const std::vector<std::string>& Values() const { return values_;}
  void AppendValue(const std::string& value) { values_.push_back(value);};

private:
  friend void to_json(Json& j, const Expression&);
  friend void from_json(const Json& j, Expression&);

  std::string term_;
  bool exclude_ = false;
  std::vector<std::string> values_;
};
void to_json(Json& j, const Expression&);
void from_json(const Json& j, Expression&);

class Document : public JsonObject {
public:
  const int64_t& DocumentId() const { return document_id_; }
  const ExpConditionMap& Conditions() const { return conditions_; }
  void SetDocumentId(const int64_t id) {document_id_ = id; }
  bool AddExpression(const Expression& exp);
  bool HasConditionExpression(const std::string& field);

private:
  friend void to_json(Json& j, const Document&);
  friend void from_json(const Json& j, Document&);
  int64_t document_id_ = -1;
  ExpConditionMap conditions_;
};
void to_json(Json& j, const Document&);
void from_json(const Json& j, Document&);

typedef std::shared_ptr<Document> RefDocument;

}  // namespace component
#endif
