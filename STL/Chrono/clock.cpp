#include <iostream>
#include <chrono>
#include <thread> // Để dùng std::this_thread::sleep_for

int main() {
    auto start = std::chrono::steady_clock::now();

    // Giả lập một công việc nặng
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto end = std::chrono::steady_clock::now();

    // Tính khoảng thời gian
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Thời gian thực thi: " << duration.count() << " ms" << std::endl;

    return 0;
}