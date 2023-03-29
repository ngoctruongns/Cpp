#include <iostream>
using namespace std;
int add(int a, int b)
{
    return a + b;
}
int main()
{
    int (*funcptr)(int, int); // function pointer declaration
    funcptr = add;            // funcptr is pointing to the add function
    int sum = funcptr(5, 5);
    int a =     1;
    std::cout << "value of sum is :" << sum << std::endl;
    std::cout << "Address of a main() function is : " <<&main<< std::endl;
    // std::cout << "Address of a add() function is : " <<&add<< std::endl;
    // std::cout << "Address of a funcptr  is : " <<funcptr<< std::endl;
    return 0;
}