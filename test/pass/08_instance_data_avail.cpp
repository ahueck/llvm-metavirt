// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

class Base {
 public:
  virtual void foo() {
  }
  virtual void print() {
  }
};

class Derived : public Base {
 public:
  void foo() override {
  }
  void print() override {
  }
};

Base base;
Derived derived;

void foo(Base* base) {
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_class_type, name: "Derived",
  // CHECK-NEXT: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_class_type, name: "Base",
  // CHECK-NEXT: Index: 1
  // CHECK-NEXT: Potential call targets:
  // CHECK-NEXT: Derived::print
  // CHECK-NEXT: Base::print
  base->print();
}