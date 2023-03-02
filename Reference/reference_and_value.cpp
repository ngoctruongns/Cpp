#include <iostream>
using namespace std;
/* Tham trị (value) truyền giá trị vào một bản sao trong hàm*/
void swap(int x, int y) {
    int temp = x;
    x = y;
    y = temp;
}

/* Tham chiếu (reference) truyền địa chỉ của biến vào hàm */
// void swap(int &x, int &y) {
//     int temp = x;
//     x = y;
//     y = temp;
// }

/* Tham chiếu dạng const để không cho thay đổi giá trị của biến */
// void swap(const int &x, int &y) {
//     int temp = x;
//     x = y;      // Báo lỗi vì x chỉ có thể đọc
//     y = temp;
// }

int main() {
    int a = 5, b = 10;
    swap(a, b);
    cout << a << " " << b << endl;
    return 0;
}