// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

// Based on cage: virtualInheritance.cpp

#include <cstdio>

struct Base {
  virtual int bar() {
    return 1;
  }  // some value
};

struct Derived : public Base {
  int bar() override {
    return 2;
  }  // some value
};

int add(Base* obj, int v) {
  // CHECK: Potential call targets:
  // CHECK-DAG: Derived::bar
  // CHECK-DAG: Base::bar
  // CHECK: -----
  return obj->bar() + v;
}

int main(int argc, char** argv) {
  int ifSwitch;
  scanf("%d", &ifSwitch);

  Base* b = nullptr;

  if (ifSwitch < 5) {
    b = new Derived();
  } else {
    b = new Base();
  }

  printf("%d\n", add(b, ifSwitch));
}
