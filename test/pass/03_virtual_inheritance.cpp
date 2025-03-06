// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// CHECK-NOT: Error

struct Base {
  virtual ~Base() = default;
  virtual void base() {}
};

struct A : virtual Base {
  virtual void a() {}
};

struct B : virtual Base {
  virtual void b() {}
};

struct Derived : A, B {};

void test_call(Base *base) {
  base->base();
}