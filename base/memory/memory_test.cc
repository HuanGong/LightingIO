
#include "scoped_ref_ptr.h"

#include <iostream>

class B : public RefCounted<B> {
  public:
    B() {
      std::cout << "b created" << std::endl;
    }
    ~B() {
      std::cout << "b created" << std::endl;
    }
  private:
    int x;
};

int main() {
  base::scoped_refptr<B> b(new B);
}
