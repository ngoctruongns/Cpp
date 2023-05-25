#include <stdio.h>

int main()
{
    int num1 = 10, num2 = 20;
    int *const ptr = &num1; // con trỏ ptr là một con trỏ hằng không thể thay đổi địa chỉ trỏ tới
    // ptr = &num2; // Lỗi! Không thể thay đổi địa chỉ mà con trỏ ptr trỏ tới

    return 0;
}
