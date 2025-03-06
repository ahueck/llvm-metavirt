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