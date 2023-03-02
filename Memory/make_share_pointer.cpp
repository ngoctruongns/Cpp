#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

using namespace std;

int main()
{
    int size;
    cout << "Enter size of the array: ";
    cin >> size;

    boost::shared_ptr<int[]> arr = boost::make_shared<int[]>(size);

    for(int i = 0; i < size; i++)
    {
        arr[i] = i;
    }

    for(int i = 0; i < size; i++)
    {
        cout << arr[i] << " ";
    }

    return 0;
}
