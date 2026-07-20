#include <iostream>
using namespace std;

/*
COMPARISON AND ANTI-PATTERNS

1. BASIC SINGLETON (01_basic_singleton.cpp)
   - NOT thread-safe
   - Manual memory management
   - Use: Simple, single-threaded applications only

2. THREAD-SAFE WITH DOUBLE-CHECKED LOCKING (02_thread_safe_singleton.cpp)
   - Thread-safe
   - More complex code
   - Still needs manual memory management
   - Use: Multi-threaded applications where explicit control is needed

3. MEYERS SINGLETON (03_meyers_singleton.cpp) ⭐ RECOMMENDED
   - Thread-safe by compiler guarantee (C++11+)
   - Simplest code
   - Automatic memory management
   - Use: Almost all modern C++ applications

4. SINGLETON WITH SHARED_PTR (04_shared_singleton.cpp)
   - Thread-safe
   - Reference counting
   - Exception-safe
   - Use: When you need multiple owning references

5. PRACTICAL EXAMPLE (05_practical_example.cpp)
   - Real-world usage patterns
   - Shows how singletons integrate with other classes
*/


// ANTI-PATTERN #1: Global variable instead of Singleton
// ❌ DON'T DO THIS
class BadConfig1 {
public:
    string setting = "default";
};

BadConfig1 globalConfig;  // Global variable - hard to test, confusing

void badApproach1() {
    globalConfig.setting = "test";  // Can be modified from anywhere
}

// ANTI-PATTERN #2: Function with static variable (looks simple but not really a Singleton)
// ❌ AVOID THIS PATTERN
BadConfig1& getBadConfig() {
    static BadConfig1 config;  // This works but looks less professional
    return config;
}


// GOOD PATTERN: Proper Singleton
class GoodConfig {
private:
    string setting;

    GoodConfig() : setting("default") {}

public:
    GoodConfig(const GoodConfig&) = delete;
    GoodConfig& operator=(const GoodConfig&) = delete;

    static GoodConfig& getInstance() {
        static GoodConfig instance;
        return instance;
    }

    void setSetting(const string& value) { setting = value; }
    string getSetting() const { return setting; }
};


// PROBLEM: Testability with Singletons
// ❌ Hard to test because singleton is global state
class DataProcessor {
public:
    void process() {
        // Can't easily mock GoodConfig for testing
        string config = GoodConfig::getInstance().getSetting();
        cout << "Processing with config: " << config << endl;
    }
};

// SOLUTION: Dependency Injection
class TestableDataProcessor {
private:
    GoodConfig& config;

public:
    // Inject dependency
    TestableDataProcessor(GoodConfig& cfg) : config(cfg) {}

    void process() {
        // Can pass mock config for testing
        string setting = config.getSetting();
        cout << "Processing with injected config: " << setting << endl;
    }
};


int main() {
    cout << "=== Singleton Patterns Comparison ===" << endl << endl;

    // Using Meyers Singleton (GOOD)
    cout << "1. Meyers Singleton (Recommended):" << endl;
    GoodConfig& cfg1 = GoodConfig::getInstance();
    cfg1.setSetting("production");
    cout << "   Setting: " << cfg1.getSetting() << endl;

    cout << "\n2. Anti-Pattern: Global Variable (Bad):" << endl;
    cout << "   // badApproach1() uses global variable - confusing!" << endl;

    cout << "\n3. Testable Approach: Dependency Injection:" << endl;
    TestableDataProcessor processor(GoodConfig::getInstance());
    processor.process();

    cout << "\n=== Key Takeaways ===" << endl;
    cout << "✓ Use Meyers Singleton for most cases (C++11+)" << endl;
    cout << "✓ Keep singletons minimal and focused" << endl;
    cout << "✓ Consider dependency injection for testability" << endl;
    cout << "✓ Avoid global variables disguised as singletons" << endl;
    cout << "✓ Don't use singletons as substitute for proper design" << endl;

    return 0;
}

/*
WHEN TO USE SINGLETON:
✓ Configuration managers
✓ Logging systems
✓ Database connection pools
✓ Thread pool managers
✓ Resource managers
✓ Application state

WHEN NOT TO USE SINGLETON:
✗ Business logic classes
✗ Services that have multiple instances
✗ Classes that need to be tested independently
✗ When you need multiple variants of the same resource
✗ Just to share global state (use proper design instead)

BEST PRACTICES:
1. Use Meyers Singleton for C++11 and later
2. Keep singletons stateless or nearly stateless
3. Use dependency injection when possible
4. Provide interface/abstract base class for testability
5. Consider lazy initialization for resources
6. Document why something needs to be a singleton
7. Make copy constructor and assignment operator deleted
*/
