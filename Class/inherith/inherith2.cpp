#include <iostream>
using namespace std;

class A
{
public:
    void foo()
    {
        cout << "This is A::foo" << endl;
    }
    void bar()
    {
        cout << "This is A::bar" << endl;
    }
};

class B : public A
{
public:
    void foo()
    {
        cout << "This is B::foo" << endl;
    }
};

int main()
{
    B b;
    b.foo(); // Gọi hàm foo() của lớp B
    b.A::foo(); // Gọi hàm foo() của lớp A
    b.bar(); // Gọi hàm bar() của lớp A
    // b.A::foo(); // Gọi hàm foo() của lớp A (khhông cần thiết, nhưng có thể làm nếu muốn)

    return 0;
}
