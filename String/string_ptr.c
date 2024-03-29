#include <stdio.h>

int main()
{
    // Pointer to constant
    const char * str1 = "Hello";
    const char * str2 = "World!";
    printf ("\nstr1: %s", str1);

    str1 = str2;
    printf ("\nstr1: %s", str1);
    int age = 34;
    
    // constant  pointer
    char * const str3 = "Pointer";
    printf ("\nstr3: %s", str3);
    // str3 = str2;  // error
    printf ("\nstr3: %s", str3);
    return 0;
}