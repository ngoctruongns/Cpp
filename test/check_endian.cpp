
#include <stdio.h>

int main()
{

    int p = 0x02;
    if (* (char *) &p == 0x02)
    {
        printf("Litter Endian");
    }
    else
    {
        printf("Big Endian");
    }

    return 0;
}
