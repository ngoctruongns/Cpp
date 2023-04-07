#include <iostream>

#include <algorithm>

using namespace std;

int main()
{
	int a = 3;

	int b = 4;

	swap(a, b);

	cout << "a = " << a << "; b = " << b;

    int arr[] = {7, 5, 3, 4, 6};

	reverse(arr, arr + 5);

	cout << "After reversing: ";

	for(int i = 0; i < 5; i++)
		cout << arr[i] << " ";

	return 0;
}