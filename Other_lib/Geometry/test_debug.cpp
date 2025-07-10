#include <stdio.h>
#include <iostream>

int main()
{
    int sum;
    for (int i=0; i<10; i++)
    {
        std::cout << "i=" << i << "\n";
        sum += i;
        std::cout << "sum= "<< sum << "\n";
    }


    return 0;
}