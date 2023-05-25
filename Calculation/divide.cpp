#include <iostream>
#include <climits>

int divide(int dividend, int divisor)
{
    printf("dividend = %d \n", dividend);
    // Xử lý trường hợp đặc biệt khi divisor = 0
    if (divisor == 0)
    {
        return INT_MAX;
    }

    // Xử lý trường hợp đặc biệt khi dividend = INT_MIN và divisor = -1
    if (dividend == INT_MIN && divisor == -1)
    {
        return INT_MAX;
    }

    // Xác định dấu của kết quả
    bool isNegative = (dividend < 0) ^ (divisor < 0);

    // Chuyển dividend và divisor về dạng dương để thực hiện thuật toán chia nhị phân
    int absDividend = labs(dividend);
    int absDivisor = labs(divisor);

    printf("absDividend = %d \n", absDividend);
    int quotient = 0;
    while (absDividend >= absDivisor)
    {
        int temp = absDivisor;
        int multiple = 1;

        // Tăng giá trị của temp và multiple cho đến khi temp lớn hơn absDividend
        while (absDividend >= (temp << 1))
        {
            temp <<= 1;
            multiple <<= 1;
            printf("Temp = %d, multiple = %d \n", temp, multiple);
        }
        printf("absDividend = %d \n", absDividend);

        absDividend -= temp;
        quotient += multiple;
        printf("quotient = %d \n", quotient);
    }

    // Áp dụng dấu cho kết quả
    if (isNegative)
    {
        quotient = -quotient;
    }

    return static_cast<int>(quotient);
}

int main()
{
    int dividend = 21;
    int divisor = 3;
    int result = divide(dividend, divisor);
    std::cout << "Quotient: " << result << std::endl;

    return 0;
}
