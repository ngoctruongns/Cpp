#include <iostream>
#include <cstring>
using namespace std;

int main()
{
    int arr[5];
    // sizeof của mảng tĩnh sẻ trả về số byte của mảng
    cout << "arr size: " << sizeof(arr) << endl;  // arr ở đây được hiểu là một đối tượng mảng
    cout << "arr num: " << sizeof(arr) / sizeof(int) << endl;

    memset(arr, 1, sizeof(arr));
    for (int j=0; j<5; j++)
    {
        cout << *(arr+j) << endl;  // ở đây arr được hiểu là con trỏ trỏ tới địa chỉ phần tử đầu tiên của mảng
    }
    return 0;
}