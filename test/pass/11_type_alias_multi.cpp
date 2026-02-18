// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

class BaseA {
 public:
  int valueA = 10;
  virtual void funcA() {
  }
  virtual ~BaseA() = default;
};

class BaseB {
 public:
  int valueB = 20;
  virtual void funcB() {
  }
  virtual ~BaseB() = default;
};

// Type aliases before inheritance
using AliasA = BaseA;
using AliasB = BaseB;

class Derived : public AliasA, public AliasB {
 public:
  void funcA() override {
  }
  void funcB() override {
  }
};

// So types are defined in IR:
Derived d_global;
BaseA a_global;

void test(BaseA* b) {
  // CHECK: Potential call targets:
  // CHECK-DAG: Derived::funcA
  // CHECK-DAG: BaseA::funcA
  // CHECK-DAG: BaseA::~BaseA
  // CHECK: -----
  b->funcA();
}

void test2(Derived* d) {
  // CHECK: Potential call targets:
  // CHECK-NEXT: Derived::funcB
  // CHECK-NEXT: -----
  d->funcB();
}
