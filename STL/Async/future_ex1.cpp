// C++ Program to illustrate the use of std::future
#include <chrono>
#include <future>
#include <iostream>
using namespace std;

// A simple function that returns some integer value
int returnTwo() { return 2; }

// driver code
int main()
{
    // creating a future object and a thread that executes
    // the function return two asynchronously
    future<int> f = async(launch::async, returnTwo);

    // getting and printing the result
    cout << f.get();

    // trying for second time
    // cout << f.get(); // error: future already retrieved

    return 0;
}