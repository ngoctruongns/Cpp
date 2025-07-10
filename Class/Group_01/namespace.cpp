// namespaces
// Namespaces allow to group entities like classes,
// objects and functions under a name.
// This way the global scope can be divided in "sub-scopes",
// each one with its own name.
#include <iostream>
using namespace std;

namespace first
{
    int var = 5;
}

namespace second
{
    double var = 3.1416;
}

int main()
{
    cout << first::var << endl;
    cout << second::var << endl;
    return 0;
}