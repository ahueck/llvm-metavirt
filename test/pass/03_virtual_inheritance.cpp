// RUN: %cpp-to-llvm %s | %apply-metavirt -S 2>&1 | %filecheck %s

struct Base {
  virtual ~Base() = default;
  virtual void base() {}
};

struct A : virtual Base {
  virtual void a() {}
  void base() override {}
};

struct B : virtual Base {
  virtual void b() {}
};

struct Derived : A, B {
  virtual void derived() {}
};

Base base;
A a;
B b;
Derived derived;

void test_call(Base *base) {
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "B",
  // CHECK-NEXT: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Derived",
  // CHECK-NEXT: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "A",
  // CHECK-NEXT: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Base",
  // CHECK-NEXT: Index: 2
  // CHECK-NEXT: Potential call targets:
  // CHECK-NEXT: A::base
  // CHECK-NEXT: Base::base
  base->base();
}

void test_proxy(Base* base) {
  A* a = dynamic_cast<A*>(base);
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "A",
  // CHECK-NEXT: Index: 3
  // CHECK-NEXT: Potential call targets:
  // CHECK-NEXT: A::a
  a->a();
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "A",
  // CHECK-NEXT: Index: 2
  // CHECK-NEXT: Potential call targets:
  // CHECK-NEXT: A::base
  a->base();

  const auto d = dynamic_cast<Derived*>(base);
  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Derived",
  // CHECK-NEXT: Index: 4
  // CHECK-NEXT: Potential call targets:
  // CHECK-NEXT: Derived::derived
  d->derived();

  // CHECK: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "A",
  // CHECK-NEXT: Class: {{.*}} = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Base",
  // CHECK-NEXT: Index: 2
  // CHECK-NEXT: Potential call targets:
  // CHECK-NEXT: A::base
  // CHECK-NEXT: Base::base
  static_cast<Base*>(a)->base();
}