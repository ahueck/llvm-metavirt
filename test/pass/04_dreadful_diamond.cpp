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
  base->base();
}

void test_proxy(Derived *derived) {
  static_cast<Base*>(static_cast<A*>(derived))->base();
  static_cast<Base*>(static_cast<B*>(derived))->base();
}