#include <iostream>
#include <limits>
#include <cmath>
#include <limits>

int main() {
    // Truy cập giá trị lớn nhất và nhỏ nhất của một số nguyên
    std::cout << "Maximum value for int: " << std::numeric_limits<int>::max() << std::endl;
    std::cout << "Minimum value for int: " << std::numeric_limits<int>::min() << std::endl;

    // Kiểm tra xem một giá trị có phải là NaN hay không
    double x = std::sqrt(-1.0);
    if (std::isnan(x)) {
        std::cout << "x is NaN" << std::endl;
    }

    // Truy cập kích thước bộ nhớ của một số nguyên
    std::cout << "Size of int: " << sizeof(int) << " bytes" << std::endl;
    std::cout << "Number of bits in int: " << std::numeric_limits<int>::digits << std::endl;

    // Kiểm tra xem một số có thể được chuyển đổi sang một kiểu dữ liệu khác hay không
    int x1 = 10;
    if (std::numeric_limits<float>::is_iec559 && (x1 > std::numeric_limits<float>::max() || x1 < std::numeric_limits<float>::min()))
    {
        std::cout << "Cannot convert x to float without loss of precision" << std::endl;
    }
    else
    {
        std::cout << "x can be converted to float without loss of precision" << std::endl;
    }

    return 0;
}
