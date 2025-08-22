// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

class Base {
 public:
  virtual void foo()   = 0;
  virtual void print() = 0;
};

class Derived : public Base {
 public:
  void print() override {
  }
};

extern Base* factory();

void foo() {
  Base* base_class = factory();
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_class_type, name: "Base",
  // CHECK-NEXT: Index: 1
  base_class->print();
}