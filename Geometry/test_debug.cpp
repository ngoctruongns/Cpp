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

    int arr[3];
    arr[0] = 1;

    return 0;
}