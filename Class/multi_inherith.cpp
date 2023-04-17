#include <iostream>
using namespace std;

class A {
public:
    void foo() {
        cout << "Hello from A\n";
    }
};

class B {
public:
    void foo() {
        cout << "Hello from B\n";
    }
};

class C : public A, public B {
public:
    void bar() {
        A::foo(); // gọi hàm foo() trong lớp cha
    }
};
