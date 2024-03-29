#include <iostream>

using namespace std;

int main()
{
    int * p1 = NULL;
    int * p2 = NULL;
    int * p3 = NULL;

    int a = 1;
    int b = 2;
    int c = 3;

    p1 = &a;
    p2 = &b;
    p3 = &c;

    // array of pointer
    int * p[] = {p1, p2, p3};

    // pointer to pointer
    int ** p_to_p  = p;

    cout << **p_to_p << endl;

    for (int i = 0; i < 3; i++)
    {
        cout << **(p_to_p + i) << endl;
    }

    return 0;
}
