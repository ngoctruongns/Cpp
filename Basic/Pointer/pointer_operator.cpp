
#include <iostream>
using namespace std;
int main()
{
    int value = 0;
    int *ptr = &value;

    cout << ptr << endl;
    // Increment operator (++)
    cout << "Before increased: " << ptr << endl;
    ptr++;
    cout << " After increased: " << ptr << endl;

    // Decrement operator (--)
    cout << "Before decreased: " << ptr << endl;
    ptr--;
    cout << " After decreased: " << ptr << endl;

    // Addition operator (+)
    ptr = ptr + 5;
    cout << ptr << endl;

    // Subtraction operator (-)
    cout << ptr << " => "  << *ptr << endl;
    cout << ptr - 5 << " => " << *(ptr - 5) << endl;
    cout << ptr - 10 << " => " << *(ptr - 10) << endl;

    // Compare operator
    int value1, value2;

    int *p1;
    int *p2;

    p1 = &value1;
    p2 = &value2;
    cout << "p1:" << p1 << endl;
    cout << "p2:" << p2 << endl;
    cout << "Is p1 less than p2?             " << (p1 < p2) << endl;
    cout << "Is p1 greater than p2?          " << (p1 > p2) << endl;
    cout << "Is p1 less than or equal p2?    " << (p1 <= p2) << endl;
    cout << "Is p1 greater than or equal p2? " << (p1 >= p2) << endl;
    cout << "Is p1 equal p2?                 " << (p1 == p2) << endl;
    cout << "Is p1 not equal p2?             " << (p1 != p2) << endl;

    // Note
    cout << "ptr: " << ptr << "\t*ptr: " << *ptr<< endl;
    cout << "*ptr++:" << (*ptr)++ << endl;
    cout << "ptr: " << ptr << "\t*ptr: " << *ptr<< endl;

    return 0;
}
    