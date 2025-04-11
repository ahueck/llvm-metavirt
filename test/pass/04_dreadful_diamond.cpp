// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

struct Base {
  virtual ~Base() = default;
  virtual void base() {}
};

struct A : Base {
  virtual void a() {}
};

struct B : Base {
  virtual void b() {}
};

struct Derived : A, B {};

void test_call(Base *base) {
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Base"
  // CHECK-NEXT: Index: 2
  base->base();
}

void test_proxy(Derived *derived) {
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Derived",
  // CHECK-NEXT: Index: 3
  derived->a();
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Derived",
  // CHECK-NEXT: Index: 3
  derived->b();
}