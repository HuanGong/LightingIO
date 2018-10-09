
#include "expression.h"

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

void to_json(Json& j, const Expression& expr) {
  j["field"] = expr.FieldName();
  j["true_exps"] = expr.TrueValues();
  j["false_exps"] = expr.FalseValues();
}

void from_json(const Json& j, Expression& expr) {
  expr.SetFieldName(j.value("field", ""));
  Json empty_json;
  const Json& true_exp = j.value("true_exps", empty_json);
  if (true_exp.is_array()) {
    expr.MutableTrueValues() = true_exp.get<std::vector<std::string> >();
  }
  const Json& false_exp = j.value("false_exps", empty_json);
  if (false_exp.is_array()) {
    expr.MutableFalseValues() = false_exp.get<std::vector<std::string> >();
  }
}


void Document::AddExpression(const Expression& exp) {
  conditions_.insert(std::make_pair(exp.FieldName(), exp));
}

void to_json(Json& j, const Document& document) {
  j["doc"] = document.DocumentId();
  const std::unordered_map<std::string, Expression>& conditions = document.Conditions();
  Json c;
  for (auto pair : conditions) {
    Json j_exp = pair.second;
    c.push_back(j_exp);
  }
  j["conditions"] = c;
}

void from_json(const Json& j, Document& document) {
  std::string id = j.value("doc", "");
  document.SetDocumentId(id);

  Json empty_json;
  const Json& j_conditions = j.value("conditions", empty_json);
  if (!j_conditions.is_array()) {
    return;
  }

  for (Json::const_iterator it = j_conditions.begin(); it != j_conditions.end(); it++) {
    component::Expression exp = *it;
    document.AddExpression(exp);
  }
}

} //end namespace
