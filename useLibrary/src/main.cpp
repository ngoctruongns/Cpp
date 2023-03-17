#include <iostream>
#include "my_class.h"


MyClass::MyClass() {}

MyClass::~MyClass() {}

void MyClass::printMessage()
{
    std::cout << "Hello from MyClass!" << std::endl;
}

// Creat child class
class ChildClass : public MyClass
{
    public:
        int age = 10;
        void print_age()
        {
            std::cout << "age1 = " << age1 << std::endl;
        }
    private:
        int age2 = 2;

};

int main()
{
    MyClass myObj;
    ChildClass childObj;
    myObj.printMessage();
    childObj.printMessage();
    childObj.print_age();
    // printf("age1: %d", childObj.age1);
    return 0;
}
