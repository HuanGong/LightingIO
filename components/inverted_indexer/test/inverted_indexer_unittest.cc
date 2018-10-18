#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include "../expression.h"
#include <sstream>
#include <string>
#include "../indexer.h"
#include "../bitmap_merger.h"
#include "../posting_field/bitmap_posting_list.h"

#define CATCH_CONFIG_MAIN //only once
#include <catch/catch.hpp>

TEST_CASE("expression_base", "[test expression and document]") {
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

TEST_CASE("indexer_file", "[load document from text file]") {
  std::ifstream ifs;
  ifs.open("test_documents.txt");
  REQUIRE(ifs.is_open());

  component::Indexer indexer;
  {
    indexer.RegisterGeneralStrField("name");
    indexer.RegisterGeneralStrField("desc");
    indexer.RegisterGeneralStrField("title");
    indexer.RegisterGeneralStrField("source");
  }

  std::string line;
  while(!ifs.eof()) {
    std::getline(ifs, line);
    component::Json j_doc = component::Json::parse(line.data(), NULL, false);
    if (j_doc.is_discarded()) {
      std::cout << "json parse failed:" << line << std::endl; continue; }
    std::shared_ptr<component::Document> doc(new component::Document);
    *doc = j_doc;
    if (doc->IsValid()) {
      indexer.AddDocument(std::move(doc));
    } else {
      std::cout << "build document from json failed" << j_doc << std::endl;
    }
  };

  indexer.BuildIndexer();

  std::ostringstream oss;
  indexer.DebugDump(oss);
  std::cout << oss.str() << std::endl;
}

TEST_CASE("bitmap_base", "[load document from text file]") {
  std::vector<int64_t> sorted_ids = {1, 2, 4, 5, 6, 8, 11, 12, 15, 22};

  component::PostingListManager manager;
  manager.SetSortedIndexedIds(std::move(sorted_ids));

  std::set<int64_t> s1 = {1, 2, 22};
  component::BitMapPostingList* p1 = manager.BuildBitMapPostingListForIds(s1);
  REQUIRE(p1->IdCount() == 3);
  std::cout << "bitcount:" << p1->BitCount() << " uint64 values:" << p1->DebugDump() <<  std::endl;
  for (uint64_t i = 0; i <= p1->BitCount(); i++) {
    std::cout << (p1->IsBitSet(i) ? "1" : "0") << "-";
  }
  std::cout << std::endl;
  component::Json j = manager.GetIdsFromPostingList(p1); 
  std::cout << j << std::endl;
  REQUIRE(s1 == manager.GetIdsFromPostingList(p1));

  std::set<int64_t> s2 = {1, 4, 4, 6, 11, 22};
  component::BitMapPostingList* p2 = manager.BuildBitMapPostingListForIds(s2);
  REQUIRE(p2->IdCount() == 5);
  component::Json j_s = manager.GetIdsFromPostingList(p2);
  std::cout << j_s << std::endl;
  REQUIRE(s2 == manager.GetIdsFromPostingList(p2));

  std::set<int64_t> s3 = {22, 22};
  component::BitMapPostingList* p3 = manager.BuildBitMapPostingListForIds(s3);
  REQUIRE(p3->IdCount() == 1);
  j_s = manager.GetIdsFromPostingList(p3);
  std::cout << j_s << std::endl;
  REQUIRE(s3 == manager.GetIdsFromPostingList(p3));

  std::set<int64_t> s4 = {11, 22};
  component::BitMapPostingList* p4 = manager.BuildBitMapPostingListForIds(s4);
  REQUIRE(p4->IdCount() == 2);
  REQUIRE(s4 == manager.GetIdsFromPostingList(p4));
  j_s = manager.GetIdsFromPostingList(p4);
  std::cout << j_s << std::endl;

}
