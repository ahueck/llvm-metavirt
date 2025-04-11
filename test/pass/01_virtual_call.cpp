// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

class Base {
 public:
  virtual void print() = 0;
};

void foo(Base* base_class) {
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_class_type, name: "Base",
  // CHECK-NEXT: Index: 0
  base_class->print();
}