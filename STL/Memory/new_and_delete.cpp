#include <iostream>
using namespace std;

int main()
{
    int i;
    cout << "Please enter an integer value: ";
    cin >> i;
    cout << "The value you entered is " << i << endl;
    int *ptr;
    if (i > 0)
    {

        ptr = new int [i];
    }
    for (int j=0; j<i; j++)
    {
        ptr[j] = j+5;
        cout << *(ptr+j) << endl;
    }
    delete ptr;
    return 0;
}