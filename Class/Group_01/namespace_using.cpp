// using
// The keyword using is used to introduce a name
// from a namespace into the current declarative region.

#include <iostream>
using namespace std;

namespace first
{
    int x = 5;
    int y = 10;
}

namespace second
{
    double x = 3.1416;
    double y = 2.7183;
}

int main()
{
    using first::x;
    using second::y;
    cout << x << endl;
    cout << y << endl;
    cout << first::y << endl;
    cout << second::x << endl;

    // // using namespace
    // using namespace first;
    // cout << x << endl;
    // cout << y << endl;
    // cout << second::x << endl;
    // cout << second::y << endl;
    return 0;
}