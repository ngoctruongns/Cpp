// #include <future>
#include <thread>
#include <iostream>

void factorial(int n, int& x) {
    unsigned long long result = 1;
    for (int i = 1; i <= n; ++i) {
        result *= i;
    }
    std::cout << "Factorial of " << n << " is " << result << std::endl;
    x = result; // Store result in the reference variable
}

int main() {
    int x;
    std::thread t1(factorial, 5, std::ref(x));

    t1.join();

    std::cout << x << std::endl;

    return 0;
}