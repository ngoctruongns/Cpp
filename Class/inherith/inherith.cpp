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

class B
{
public:
    void foo()
    {
        cout << "This is B::foo" << endl;
    }
};

class C : public A, public B
{
public:
    void callAfoo()
    {
        A::foo();
    }

    void callBfoo()
    {
        B::foo();
    }
};

int main()
{
    C c;
    c.callAfoo();
    c.callBfoo();
    c.bar();
    return 0;
}
