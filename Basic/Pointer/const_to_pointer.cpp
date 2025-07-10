#include <stdio.h>

int main()
{
    const int *ptr; // con trỏ ptr trỏ tới một giá trị số nguyên không thể thay đổi
    int num = 10;
    ptr = &num; // OK, gán địa chỉ của num cho con trỏ ptr
    // *num = 20;  // Lỗi! Không thể thay đổi giá trị mà con trỏ ptr trỏ tới

    return 0;
}
