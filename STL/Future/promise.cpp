#include <future>
#include <iostream>
#include <thread>

int factorial(std::future<int>& f)
{
    std::cout << "Function start..." << std::endl;
    int res = 1;
    int N = f.get(); // Get the value from the future
    std::cout << "Calculating factorial of " << N << std::endl;
    for (int i = 1; i <= N; ++i) {
        res *= i;
    }
    std::cout << "Factorial of " << N << " is " << res << std::endl;
    return res; // Return the result
}


int main() {

    int x;
    std::promise<int> p;
    std::future<int> f = p.get_future();
    std::future<int> fut = std::async(std::launch::async, factorial, std::ref(f));
    std::cout << "Main thread start sleep ... " << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate some delay
    std::cout << "Main thread end sleep ... " << std::endl;
    p.set_value(5); // Set the value to be used in the factorial calculation

    x = fut.get(); // Wait for the async task to complete
    std::cout << "Result from async: " << x << std::endl;

    return 0;
}