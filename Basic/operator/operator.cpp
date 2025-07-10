#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;
int main()
{
    unsigned int A = 1226851137;
    unsigned int B = 32;
    unsigned int C;
    unsigned char D;

    // Bitwise Operators
    C = A >> 8;
    D = A % 256;
    cout << "size of A: " << sizeof(A)<< endl;
    cout << C << endl;
    printf("D = %d", D);

    return 0;
}