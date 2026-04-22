#include <iostream>
#include <array>
using namespace std;

int main()
{
    array<int, 5> arr = {0,1,2,3,4};

    // Testing operator[]
    for (int i = 0; i < 5; i++)
        cout << arr[i] << " ";
    cout << endl;

    // Testing at() method
    cout << arr.at(2) << endl;

    // This will throw an exception
    try {
        cout << arr.at(10) << endl;
    } catch (const out_of_range& e) {
        cerr << e.what() << endl;
    }

    return 0;
}