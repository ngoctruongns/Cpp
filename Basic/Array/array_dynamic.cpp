#include <iostream>
#include <cstring>
using namespace std;

int main()
{
    int i = 5;
    int* ptr = new int [i];

    // Hàm sizeof() với mảng động sẽ trả về kích thước của con trỏ
    cout << "arr size: " << sizeof(ptr) << endl;
    // arr size: 8

    // memset(ptr, 1, sizeof(ptr)); // Chỉ đặt cho 8 byte
    // 16843009
    // 16843009
    // 0
    // 0
    // 0
    memset(ptr, 1, sizeof(int) * i); // đặt cho tất cả 4x5 = 20 byte
    // 16843009
    // 16843009
    // 16843009
    // 16843009
    // 16843009

    for (int j=0; j<i; j++)
    {
        // ptr[j] = j+5;
        cout << *(ptr+j) << endl;
    }
    delete ptr;
    return 0;
}