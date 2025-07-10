#include <iostream>

#include <algorithm>

using namespace std;

int main()
{
    /* Min, Max */
    int a = 3;

    int b = 4;

    cout << "Min = " << min(a, b) << "; Max = " << max(a, b) << endl;

    /* Sort */
    int arr[] = {7, 5, 3, 4, 6};
	sort(arr, arr + 5); // sort from &arr to &(arr + 5) -1

	cout << "After sorting: ";

	for(int i = 0; i < 5; i++)
		cout << arr[i] << " ";

    return 0;
}