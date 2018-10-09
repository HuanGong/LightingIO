#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include "expression.h"
#include <strstream>

#define CATCH_CONFIG_MAIN //only once
#include <catch/catch.hpp>

TEST_CASE("expression test", "[test expression and document]") {
  component::Expression expr;
  std::string field("name");
  expr.SetFieldName(field);
  auto& true_condition = expr.MutableTrueValues();
  true_condition.push_back("world");
  true_condition.push_back("hello");

  auto& false_condition = expr.MutableFalseValues();
  false_condition.push_back("baby");
  false_condition.push_back("xiao");

  component::Json out = expr;
  //std::cout << out << std::endl;

  std::ostringstream oss;
  oss << out;
  std::cout << oss.str() << std::endl;

  component::Expression resume_exp;
  resume_exp = out;

  std::ostringstream resume_oss;
  resume_oss << " field:" << resume_exp.FieldName();
  resume_oss << " \n true_exps:";
  for (auto& v : resume_exp.TrueValues()) {
    resume_oss << v << ",";
  }
  resume_oss << " \n false_exps:";
  for (auto& v : resume_exp.FalseValues()) {
    resume_oss << v << ",";
  }
  std::cout << " after resume:\n" << resume_oss.str() << std::endl;
}

TEST_CASE("document_base", "[test document basic funcation]") {

  component::Document doc;

  component::Expression expr;
  std::string field("name");
  expr.SetFieldName(field);
  auto& true_condition = expr.MutableTrueValues();
  true_condition.push_back("true_value01");
  auto& false_condition = expr.MutableFalseValues();
  false_condition.push_back("false_value01");

  doc.SetDocumentId("1");
  doc.AddExpression(expr);

  expr.SetFieldName("desc");
  doc.AddExpression(expr);


  component::Json j_doc = doc;
  std::cout << j_doc << std::endl;

}
