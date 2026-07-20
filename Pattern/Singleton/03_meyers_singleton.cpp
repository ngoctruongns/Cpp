#include <iostream>
#include <thread>
using namespace std;

// MEYERS SINGLETON (C++11 and later)
// This is the BEST approach - automatically thread-safe by compiler
// The static local variable is guaranteed to be initialized only once by the compiler

class Logger {
private:
    // Private constructor
    Logger() {
        cout << "Logger instance created" << endl;
    }

public:
    // Delete copy operations
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Get singleton instance using static local variable
    // Compiler guarantees thread-safe initialization (C++11 and later)
    static Logger& getInstance() {
        static Logger instance;  // Created only once, thread-safe
        return instance;
    }

    void log(const string& message) {
        cout << "[LOG] " << message << endl;
    }
};


int main() {
    cout << "=== Meyers Singleton (Best Approach) ===" << endl;

    // Create multiple threads accessing the singleton
    auto writeLog = [](int id) {
        for (int i = 0; i < 3; ++i) {
            Logger::getInstance().log("Message from thread " + to_string(id));
        }
    };

    thread t1(writeLog, 1);
    thread t2(writeLog, 2);
    thread t3(writeLog, 3);

    t1.join();
    t2.join();
    t3.join();

    cout << "\nAll threads completed!" << endl;

    return 0;
}

/*
OUTPUT:
=== Meyers Singleton (Best Approach) ===
Logger instance created
[LOG] Message from thread 1
[LOG] Message from thread 1
[LOG] Message from thread 1
[LOG] Message from thread 2
[LOG] Message from thread 2
[LOG] Message from thread 2
[LOG] Message from thread 3
[LOG] Message from thread 3
[LOG] Message from thread 3

All threads completed!

ADVANTAGES:
- Simplest code
- Automatically thread-safe (C++11+)
- No manual memory management needed
- Automatic cleanup (destructor called at program exit)
- Returns reference (no pointer usage)

DISADVANTAGES:
- None significant! This is the recommended approach.

WHY IT WORKS:
According to C++11 standard (Magic Statics):
- Static local variables are initialized exactly once
- The compiler inserts necessary synchronization code
- Thread-safe by language guarantee
*/
