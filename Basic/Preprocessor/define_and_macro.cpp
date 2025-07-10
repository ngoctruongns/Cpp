/**
 * @file define_and_macro.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-03-18
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <iostream>
// bộ xử lý trước tiên sẽ thay thế lệnh #include bằng nội dung của file mà bạn include nó.
using namespace std;
/**
 * @brief Macro
 * #define identifier
 * #define identifier substitution_text
 *
 */
#define MY_NAME "Alex"

int main()
{
    #ifdef MY_NAME
    std::cout << "Joe\n";
    std::cout << "My name is: " << MY_NAME << endl; // if MY_NAME is defined, compile this code
    #endif

    #ifndef MY_NAME

    std::cout << "My name is: " << "Not Define" << std::endl;
    #endif

    #define MY_VAL

    #ifdef MY_VAL
    int abc = 10;
    std::cout << "My value is: " << abc << endl;
    #endif


    // Một cách sử dụng phổ biến của biên dịch có điều kiện liên quan đến việc sử dụng #if 0
    // để loại trừ một khối code khỏi được biên dịch (như thể nó nằm trong một khối comments)
    #if 0 // Don't compile anything starting here
    std::cout << "Bob\n";
    std::cout << "Steve\n";
    #endif // until this point

    return 0;
}