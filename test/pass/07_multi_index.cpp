// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

class Base {
public:
  virtual ~Base() {}
  virtual void a() = 0;
  virtual void print() = 0;
  virtual void b() = 0;
};

void foo(Base* base_class) {
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_class_type, name: "Base",
  // CHECK-NEXT: Index: 3
  base_class->print();
}