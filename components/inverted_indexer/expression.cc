#include "expression.h"

namespace component {

using Json = nlohmann::json;

void to_json(Json& j, const Expression& expr) {
  j["exps"] = expr.Values();
  j["field"] = expr.FieldName();
  j["excl"] = expr.IsExclude();
}

void from_json(const Json& j, Expression& expr) {
  expr.SetValid(true);
  try {
    j["field"].get_to(expr.term_);
    if (expr.FieldName().empty()) {
      expr.SetValid(false);
      return;
    }

    Json j_null;
    const Json& j_exclude = j.value("excl", j_null);
    const Json& j_expressions = j.value("exps", j_null);
    if (!j_exclude.is_boolean() || !j_expressions.is_array()) {
      expr.SetValid(false);
      return;
    }
    expr.SetExclude(j_exclude);
    j_expressions.get_to(expr.MutableValues());
  } catch (...) {
    expr.SetValid(false);
  }
}

bool Document::AddExpression(const Expression& exp) {
  return conditions_.insert(std::make_pair(exp.FieldName(), exp)).second;
}

bool Document::HasConditionExpression(const std::string& field) {
  return conditions_.end() != conditions_.find(field);
}

void to_json(Json& j, const Document& document) {
  j["doc"] = document.DocumentId();
  Json conditions;
  for (auto pair : document.Conditions()) {
    Json j_exp = pair.second;
    conditions.push_back(j_exp);
  }
  j["cond"] = conditions;
}

void from_json(const Json& j, Document& document) {
  const static Json j_null;
  try {
    const Json& doc_id = j.value("doc", j_null);
    document.SetDocumentId(doc_id.get<int64_t>());

    const Json& j_conditions = j.value("cond", j_null);
    if (!j_conditions.is_array()) {
      document.SetValid(false);
      return;
    }
    for (Json::const_iterator it = j_conditions.begin(); it != j_conditions.end(); it++) {
      component::Expression exp = *it;
      if (!exp.IsValid() || !document.AddExpression(exp)) {
        document.SetValid(false);
        return;
      }
    }
    document.SetValid(true);
  } catch (...) {
    document.SetValid(false);
  }
}

}  // namespace component
