// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

class Base {
 public:
  virtual void print() = 0;
};

class Derived : public Base {
 public:
  void print() override {
  }
};

void foo(Base* base_class) {
  base_class = new Derived();
  base_class->print();
}