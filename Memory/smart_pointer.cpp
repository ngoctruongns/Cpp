#include <boost/shared_ptr.hpp>
#include <iostream>
using namespace std;

int main()
{
    int i;
    cout << "Please enter an integer value: ";
    cin >> i;
    cout << "The value you entered is " << i << endl;
    if (i > 0)
    {
        boost::shared_ptr<int[]> ptr(new int[i]);

        for (int j = 0; j < i; j++)
        {
            ptr[j] = j + 5;
            cout << ptr[j] << endl;
        }
    }
    else
    {
        cout << "The value you entered is not positive. Please enter a positive integer value." << endl;
    }
    // Không cần dùng delete
    return 0;
}