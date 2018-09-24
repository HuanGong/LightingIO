//
// Created by gh on 18-9-23.
//

#ifndef LIGHTINGIO_JSON_CLASS_META_H
#define LIGHTINGIO_JSON_CLASS_META_H

namespace component {
namespace sl {

class JsonClassMeta {
public:
	explicit JsonClassMeta()
		:is_null_(true) {
	};
	inline bool IsNull() const {return is_null_;};
	inline void SetNull(bool null) {is_null_ = null;}
protected:
	bool is_null_;
};

}}
#endif //LIGHTINGIO_JSON_CLASS_META_H
