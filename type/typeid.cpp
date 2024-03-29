#include <iostream>
#include <typeinfo>

using namespace std;
int main()
{
    int *prt = NULL;
    int speed = 100;
    prt = &speed;
    
    cout << typeid(&prt).name() << std::endl;

    return 0;
}