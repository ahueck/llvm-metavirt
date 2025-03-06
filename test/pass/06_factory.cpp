// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

class Base {
public:
  virtual void print() = 0;
};

class Derived : public Base {
public:
  void print() override {}
};

extern Base *factory();

void foo() {
  Base* base_class = factory();
  base_class->print();
}