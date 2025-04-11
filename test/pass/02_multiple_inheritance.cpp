// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

struct A {
  virtual void a() {}
};

struct B {
  virtual void b() {}
};

struct Derived : A, B {
  void a() override { b(); }

  void b() override { a(); }
};

void foo(Derived* d) {
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Derived",
  // CHECK-NEXT: Index: 1
  d->b();
}