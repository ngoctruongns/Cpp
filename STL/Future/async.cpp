#include <future>
#include <iostream>

void factorial(int n, int& x) {
    unsigned long long result = 1;
    for (int i = 1; i <= n; ++i) {
        result *= i;
    }
    std::cout << "Factorial of " << n << " is " << result << std::endl;
    x = result; // Store result in the reference variable
}

int factorial2(int n) {
    unsigned long long result = 1;
    for (int i = 1; i <= n; ++i) {
        result *= i;
    }
    return result; // Return the result
}

int main() {
    int x;
    std::future<void> fut = std::async(std::launch::async, factorial, 5, std::ref(x));
    fut.get(); // Wait for the async task to complete

    std::cout << x << std::endl;

    std::future<int> fut2 = std::async(factorial2, 5);
    int result = fut2.get(); // Get the result from the future
    std::cout << "Factorial of 5 is " << result << std::endl;

    return 0;
}