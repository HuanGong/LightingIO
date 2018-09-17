//
// Created by gh on 18-9-16.
//

#ifndef LIGHTINGIO_SCOPED_GUARD_H
#define LIGHTINGIO_SCOPED_GUARD_H

#include <functional>

namespace base {

class ScopedGuard {
public:
	ScopedGuard(std::function<void()> func)
		: func_(func) {
	}
	~ScopedGuard() {
		if (!dismiss_ && func_) {
			func_();
		}
	}
	void DisMiss() {dismiss_ = true;}
private:
	bool dismiss_ = false;
	std::function<void()> func_;
};

}
#endif //LIGHTINGIO_SCOPED_GUARD_H
