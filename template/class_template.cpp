#include <iostream>

template <typename T>

class  MyClass
{
private:
    T value;
public:
     MyClass(T val): value(val) {};
    ~ MyClass();
};

