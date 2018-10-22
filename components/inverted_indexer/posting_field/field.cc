#include "field.h"
#include <iostream>

namespace component {

void FieldEntity::AddWildcardsId(DocIdType id) {
  wildcards.insert(id);
}

void FieldEntity::AddIdForAssign(const std::string assign, bool exclude, DocIdType id) {
  exclude ? exclude_ids[assign].insert(id) : include_ids[assign].insert(id);
}

void to_json(Json& json, const FieldEntity& entity) {
	json["wildcards"] = entity.wildcards;
	json["include_ids"] = entity.include_ids;
	json["exclude_ids"] = entity.exclude_ids;
}

Field::Field(std::string name)
		: field_name_(name) {
}

Field::~Field() {
}

void Field::SetWildcardsPostingList(BitMapPostingList* pl) {
  wildcards_pl_ = pl;
}

} //end component
