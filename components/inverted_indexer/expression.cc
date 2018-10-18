#include "expression.h"

namespace component {

using Json = nlohmann::json;

void to_json(Json& j, const Expression& expr) {
  j["exps"] = expr.Values();
  j["field"] = expr.FieldName();
  j["exclude"] = expr.IsExclude();
}

void from_json(const Json& j, Expression& expr) {
  expr.SetFieldName(j.value("field", ""));
  if (expr.FieldName().empty()) {
    expr.SetValid(false);
    return;
  }
  Json empty_json;
  const Json& true_exp = j.value("exps", empty_json);
  const Json& j_exclude = j.value("exclude", empty_json);
  if (!j_exclude.is_boolean()) {
    expr.SetValid(false);
    return;
  }
  expr.SetExclude(j_exclude);
  if (!true_exp.is_array()) {
    expr.SetValid(false);
    return;
  }
  expr.MutableValues() = true_exp.get<std::vector<std::string>>();
  expr.SetValid(true);
}

void Document::AddExpression(const Expression& exp) {
  conditions_.insert(std::make_pair(exp.FieldName(), exp));
}

bool Document::HasConditionExpression(const std::string& field) {
  return conditions_.end() != conditions_.find(field);
}

void to_json(Json& j, const Document& document) {
  j["doc"] = document.DocumentId();
  const std::unordered_map<std::string, Expression>& conditions =
      document.Conditions();
  Json c;
  for (auto pair : conditions) {
    Json j_exp = pair.second;
    c.push_back(j_exp);
  }
  j["conditions"] = c;
}

void from_json(const Json& j, Document& document) {
  Json empty_json;

  const Json& doc_id = j.value("doc", empty_json);
  document.SetDocumentId(doc_id.get<int64_t>());

  const Json& j_conditions = j.value("conditions", empty_json);
  if (!j_conditions.is_array()) {
    document.SetValid(false);
    return;
  }
  for (Json::const_iterator it = j_conditions.begin(); it != j_conditions.end();
       it++) {
    component::Expression exp = *it;
    document.AddExpression(exp);
  }
  document.SetValid(true);
}

}  // namespace component
