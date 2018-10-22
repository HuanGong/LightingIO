#include <catch/catch.hpp>

#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "../indexer.h"
#include "../expression.h"
#include "../bitmap_merger.h"
#include "../posting_field/bitmap_posting_list.h"

TEST_CASE("expression_base", "[test expression and document]") {
  component::Expression expr;
  std::string field("name");
  expr.SetFieldName(field);
  expr.SetExclude(false);
  auto& condition = expr.MutableValues();
  condition.push_back("world");
  condition.push_back("hello");

  component::Json out = expr;

  std::cout << "  to_json:" << out << std::endl;

  component::Expression resume_exp = out;

  out = resume_exp;
  std::cout << "from_json:" << out << std::endl;
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
  std::cout << "  to_json:" << j_doc << std::endl;
  component::Document resume = j_doc;
  j_doc = resume;
  std::cout << "from_json:" << j_doc << std::endl;
}

TEST_CASE("indexer_file", "[load document from text file]") {
  std::ifstream ifs;
  ifs.open("test_documents.txt");
  REQUIRE(ifs.is_open());
  component::DefaultDelegate d;
  {
    d.RegisterGeneralStrField("name");
    d.RegisterGeneralStrField("desc");
    d.RegisterGeneralStrField("title");
    d.RegisterGeneralStrField("source");
  }

  component::Indexer indexer(&d);

  std::string line;
  while (!ifs.eof()) {
    std::getline(ifs, line);
    component::Json j_doc = component::Json::parse(line.data(), NULL, false);
    if (j_doc.is_discarded()) {
      std::cout << "json parse failed: content:" << line << std::endl;
      continue;
    }

    std::cout << "handle doc:" << j_doc << std::endl;

    std::shared_ptr<component::Document> doc(new component::Document);
    *doc = j_doc;
    if (doc->IsValid()) {
      indexer.AddDocument(std::move(doc));
    } else {
      std::cout << "build document from json failed" << j_doc << std::endl;
    }
  };

  indexer.BuildIndexer();

  //typedef std::map<std::string, std::vector<std::string>> IndexerQuerys;
  component::IndexerQuerys querys = {
    {"source", {"2", "desc_unknown"}},
    {"desc", {"desc01", "desc_unknown"}},
    //{"name", {"namexx", "name_unknown"}},
    {"title", {"title01", "title_unknown"}}
  };
  component::Json query_request = querys;
  component::Json query_result = indexer.UnifyQuery(querys);

  std::ostringstream oss;
  indexer.DebugDump(oss);
  std::cout << oss.str() << std::endl;
  std::cout << "query result:" << query_result << std::endl;
  std::cout << "query request:" << query_request << std::endl;
}

TEST_CASE("bitmap_base", "[load document from text file]") {
  std::vector<int64_t> sorted_ids = {1, 2, 4, 5, 6, 8, 11, 12, 15, 22};

  component::PostingListManager manager;
  manager.SetSortedIndexedIds(std::move(sorted_ids));

  std::set<int64_t> s1 = {1, 2, 22};
  component::BitMapPostingList* p1 = manager.BuildBitMapPostingListForIds(s1);
  REQUIRE(p1->IdCount() == 3);
  std::cout << "bits info:" << p1->DumpBits() << std::endl;

  component::Json j = manager.GetIdsFromPostingList(p1);
  std::cout << "id from bitmap:" << j << std::endl;
  REQUIRE(s1 == manager.GetIdsFromPostingList(p1));

  std::set<int64_t> s2 = {1, 4, 4, 6, 11, 22};
  component::BitMapPostingList* p2 = manager.BuildBitMapPostingListForIds(s2);
  REQUIRE(p2->IdCount() == 5);
  component::Json j_s = manager.GetIdsFromPostingList(p2);
  std::cout << "id from bitmap:" << j_s << std::endl;
  REQUIRE(s2 == manager.GetIdsFromPostingList(p2));

  std::set<int64_t> s3 = {22, 22};
  component::BitMapPostingList* p3 = manager.BuildBitMapPostingListForIds(s3);
  REQUIRE(p3->IdCount() == 1);
  j_s = manager.GetIdsFromPostingList(p3);
  std::cout << "id from bitmap:" << j_s << std::endl;
  REQUIRE(s3 == manager.GetIdsFromPostingList(p3));

  std::set<int64_t> s4 = {11, 22};
  component::BitMapPostingList* p4 = manager.BuildBitMapPostingListForIds(s4);
  REQUIRE(p4->IdCount() == 2);
  REQUIRE(s4 == manager.GetIdsFromPostingList(p4));
  j_s = manager.GetIdsFromPostingList(p4);
  std::cout << "id from bitmap:" << j_s << std::endl;
}
