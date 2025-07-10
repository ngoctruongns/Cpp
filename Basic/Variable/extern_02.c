// module2.c

#include <stdio.h>

extern int var;
extern int var2;

int main() 
{
    printf("Global variable value: %d\n", var);
    var = 15;
    printf("Global variable value: %d\n", var2);
    var2 = 45;
    return 0;
}