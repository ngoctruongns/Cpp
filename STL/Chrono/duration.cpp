#include <chrono>
#include <iostream>

int main() {
    std::chrono::milliseconds ms(1100); // 1100 mili giây
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(ms);
    std::cout << ms.count() << "ms tương đương với " << s.count() << " giây\n"; // 1 giây

    std::chrono::seconds s2(5); // 5 giây

    // You can use +, -, *, / operators with durations
    auto s3 = s + s2;

    std::cout << "Tổng thời gian là: " << s3.count() << " giây\n"; // 6 giây
    return 0;
}