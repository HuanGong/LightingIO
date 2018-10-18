#include "field.h"
#include <iostream>

namespace component {

void to_json(Json& j, const PostingList& pl) {
	j["wildcards"] = pl.wildcards;
	j["include_ids"] = pl.include_pl;
	j["exclude_ids"] = pl.exclude_pl;
}


Field::Field(std::string name)
		: field_name_(name) {
}

Field::~Field() {
}

bool Field::ResolveExpression(const Expression& expr, const Document* doc) {
	if (!expr.IsValid()) {
		return false;
	}
	int64_t doc_id = doc->DocumentId();

	bool is_exclude = expr.IsExclude();
	for (const auto& v : expr.Values()) {
		AddPostingListValues(v, is_exclude, doc_id);
	}
	if (expr.Values().empty()) {
		AddWildcards(doc_id);
	}
	return true;
}

void Field::AddPostingListValues(const std::string& value, bool is_exclude, int64_t doc) {
	if (is_exclude) {
		posting_list_.exclude_pl[value].insert(doc);
	} else {
		posting_list_.include_pl[value].insert(doc);
	}
	std::cout << " add pl values:" << value << " doc:" << doc << std::endl;
}

void Field::AddWildcards(int64_t wc_doc) {
	posting_list_.wildcards.insert(wc_doc);
}

void Field::SetWildcardsPostingList(BitMapPostingList* pl) {
  wildcards_pl_ = pl;
}

} //end component
