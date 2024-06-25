#include <iostream>
#include <cstdio>
#include <chrono>

int main() {
    // Đo thời gian cho std::cout
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        std::cout << "Hello, World!" << std::endl;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> cout_duration = end - start;

    // Đo thời gian cho printf
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        printf("Hello, World!\n");
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> printf_duration = end - start;
    std::cout << "std::cout duration: " << cout_duration.count() << " seconds" << std::endl;
    printf("printf duration: %f seconds\n", printf_duration.count());

    return 0;
}
