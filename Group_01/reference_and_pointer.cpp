#include<stdio.h>

void hoan_vi(int &a, int &b)
{
    int temp;
    printf("Truoc hoan vi a= %d, b= %d\n",a,b);
    temp= a;
    a= b;
    b= temp;
    printf("Sau hoan vi a= %d, b= %d\n",a,b);

}

// gap doi gia tri truyen vao
void gap_doi(int *&a)
{
    printf("Truoc gap doi a= %p\n",a);
    *a *= 2;
    printf("Sau gap doi   a= %p\n",a);
}

// Tao doi tuong
struct odomData
{
    int x = 3;
    int theta = 2;
};

int main()
{
    int a,b;
    a= 2;
    b= 0x1A;

    // reference
    int &ra = a;

    hoan_vi(a,b);
    printf("a= %d\n",a);
    printf("ra= %d\n",ra);
    printf("b= %d\n",b);
    printf("&a= %p\n",&a);
    printf("&ra= %p\n",&ra);

    // pointer
    int *pa, *pb ;
    pa = &a;
    pb = &b;

    hoan_vi(*pa,*pb);
    printf("a= %d\n",a);
    printf("*pa= %d\n",*pa);
    printf("b= %d\n",b);
    printf("*pb= %d\n",*pb);
    printf("&a= %p\n",&a);
    printf("pa= %p\n",pa);
    printf("&b= %p\n",&b);
    printf("pb= %p\n",pb);

    // pointer and function
    gap_doi(pa);
    printf("a= %d\n",a);
    printf("b= %d\n",b);

    return 0;
}