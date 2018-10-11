#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include "expression.h"
#include <sstream>
#include "indexer.h"

#define CATCH_CONFIG_MAIN //only once
#include <catch/catch.hpp>

TEST_CASE("expression test", "[test expression and document]") {
  component::Expression expr;
  std::string field("name");
  expr.SetFieldName(field);
  expr.SetExclude(false); 
  auto& condition = expr.MutableValues();
  condition.push_back("world");
  condition.push_back("hello");

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
  for (auto& v : resume_exp.Values()) {
    resume_oss << v << ",";
  }
  std::cout << " after resume:\n" << resume_oss.str() << std::endl;
}

TEST_CASE("document_base", "[test document basic funcation]") {

  component::Document doc;

  component::Expression expr;
  std::string field("name");
  expr.SetFieldName(field);
  expr.SetExclude(false); 
  auto& condition = expr.MutableValues();
  condition.push_back("true_value01");

  doc.SetDocumentId(1);
  doc.AddExpression(expr);

  expr.SetFieldName("desc");
  doc.AddExpression(expr);

  component::Json j_doc = doc;
  std::cout << j_doc << std::endl;
}

TEST_CASE("indexer", "[try basic indexer]") {

  component::IndexerBuilder builder;

  for (int i = 0; i < 5; i++) {
    std::shared_ptr<component::Document> doc(new component::Document);
    doc->SetDocumentId(i);

    component::Expression expr;
    std::string term("name");
    expr.SetExclude(false);
    expr.SetFieldName(term);
    auto& condition = expr.MutableValues();
    condition.push_back("include_v1");
    condition.push_back("name_include_" + std::to_string(i));

    doc->AddExpression(expr);

    expr.SetFieldName("desc");
    condition.clear();
    condition.push_back("desc_include_" + std::to_string((i%2)*100));
    doc->AddExpression(expr);

    expr.SetFieldName("age");
    condition.clear();
    doc->AddExpression(expr);

    expr.SetFieldName("movie");
    condition.clear();
    expr.SetExclude(true);
    condition.push_back("exclude_" + std::to_string(i % 2));
    doc->AddExpression(expr);

    builder.AddDocument(std::move(doc));
  }
  std::shared_ptr<component::Document> doc(new component::Document);
  doc->SetDocumentId(1000);
  builder.AddDocument(std::move(doc));

  auto indexer = builder.Build();
  indexer->DebugDump();
}
