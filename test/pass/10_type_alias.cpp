// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// Test to verify that we look through "typedef" aliases

class BaseA {
 public:
  int valueA = 10;
  virtual void funcA() {
  }
  virtual ~BaseA() = default;
};

using AliasA = BaseA;

class Derived : public AliasA {
 public:
  void funcA() override {
  }
};

// So types are defined in IR:
Derived d;
BaseA a;

void test(BaseA* d) {
  // CHECK: Potential call targets:
  // CHECK-DAG: Derived::funcA
  // CHECK-DAG: BaseA::funcA
  // CHECK-DAG: BaseA::~BaseA
  // CHECK: -----
  d->funcA();
}

void test2(Derived* d) {
  // CHECK: Potential call targets:
  // CHECK-NEXT: Derived::funcA
  // CHECK-NEXT: -----
  d->funcA();
}