#ifndef _LT_COMPONENT_INVERTED_INDEX_INDEXER_H_
#define _LT_COMPONENT_INVERTED_INDEX_INDEXER_H_

#include "expression.h"
#include "posting_field/general_str_field.h"
#include "posting_field/posting_list_manager.h"

namespace component {

typedef std::set<std::string> FieldsSet;
typedef std::function<FieldPtr()> FieldCreator;

typedef std::map<std::string, std::vector<std::string>> IndexerQuerys;

/* EntitysMeta is a middle infomation and data using for build a postinglinst
 * Indexer It can be destory after a indexer finish build for save memory, or
 * save it for more convenience debugging;   */
typedef struct {
  std::set<int64_t> all_documens_ids_;
  std::vector<RefDocument> all_documents_;
  std::unordered_map<std::string, FieldEntity> field_entitys_;
} EntitysMeta;

class Indexer {
public:
  class Delegate {
  public:
    virtual const std::set<std::string> AllRegisterFields() const = 0;
    virtual FieldPtr CreatePostinglistField(const std::string& name) = 0;
  };

public:
  Indexer(Delegate* d);

  std::set<int64_t> UnifyQuery(const IndexerQuerys& query);

  void AddDocument(const RefDocument&& document);
  void BuildIndexer();

  void DebugDump(std::ostringstream& oss);
  void DebugDumpField(const std::string& field, std::ostringstream& oss);

protected:
  Field* GetPostingListField(const std::string& field);
private:
  Delegate* delegate_ = NULL;
  bool build_finished_ = false;

  bool keep_docments_;
  std::unique_ptr<EntitysMeta> entitys_meta_;

  std::unique_ptr<PostingListManager> pl_manager_;
  std::unordered_map<std::string, FieldPtr> indexer_table_;
};

class DefaultDelegate : public Indexer::Delegate {
public:
  void RegisterGeneralStrField(const std::string field);
  void RegisterFieldWithCreator(const std::string& f, FieldCreator creator);

  const std::set<std::string> AllRegisterFields() const override;
  FieldPtr CreatePostinglistField(const std::string& name) override;
private:
  FieldsSet indexer_fields_;
  std::unordered_map<std::string, FieldCreator> creators_;
};


}  // namespace component
#endif
