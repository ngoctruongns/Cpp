#include <iostream>
#include <future>
#include <chrono>
#include <thread>

int doSomething() {
    std::cout << "start do something\n";
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "do something done!\n";
    return 12;
}

int main()
{
    std::cout<<"Hello World\n";
    // std::future<int> fut = std::async(std::launch::async, doSomething);
    std::future<int> fut = std::async(std::launch::deferred, doSomething);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Get value from future\n";

    int val = fut.get();
    std::cout << "val: " << val << std::endl;

    return 0;
}