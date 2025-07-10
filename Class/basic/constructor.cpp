#include <iostream>

class MyClass  // The Class
{
private:        // Access specifier
    int value;  // Field 
public:
    MyClass() {  // Constructor
        std::cout << "Hello World" << std::endl;
    };
    void displayValue()     // Method
    {
        std::cout << value << std::endl;
    }
    ~MyClass() {};      // Destructor
};

class MyClass1
{
private:
    int value;
public:
    MyClass1();
    ~MyClass1() {};
};

MyClass1::MyClass1()
{
    std::cout << "This is class 1"<< std::endl;
}

int main(void)
{
    MyClass obj ;
    MyClass1 obj1;
    return 0;
}