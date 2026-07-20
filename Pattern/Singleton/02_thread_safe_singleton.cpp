#include <iostream>
#include <mutex>
#include <thread>
using namespace std;

// THREAD-SAFE SINGLETON WITH DOUBLE-CHECKED LOCKING
// This approach uses mutex to ensure thread safety

class DatabaseConnection {
private:
    static DatabaseConnection* instance;
    static mutex createMutex;

    // Private constructor
    DatabaseConnection() {
        cout << "DatabaseConnection created (Thread-safe)" << endl;
    }

public:
    // Delete copy operations
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    // Thread-safe getInstance using Double-Checked Locking
    static DatabaseConnection* getInstance() {
        // First check - no lock (performance)
        if (instance == nullptr) {
            // Lock before creating
            lock_guard<mutex> lock(createMutex);

            // Second check - inside lock (correctness)
            if (instance == nullptr) {
                instance = new DatabaseConnection();
            }
        }
        return instance;
    }

    void query(const string& sql) {
        cout << "Executing: " << sql << endl;
    }
};

// Static member initialization
DatabaseConnection* DatabaseConnection::instance = nullptr;
mutex DatabaseConnection::createMutex;


int main() {
    cout << "=== Thread-Safe Singleton (Double-Checked Locking) ===" << endl;

    // Create multiple threads trying to get singleton instance
    auto createInstance = []() {
        DatabaseConnection* db = DatabaseConnection::getInstance();
        db->query("SELECT * FROM users");
    };

    // Start 5 threads
    thread t1(createInstance);
    thread t2(createInstance);
    thread t3(createInstance);
    thread t4(createInstance);
    thread t5(createInstance);

    // Wait for all threads to complete
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    cout << "\nAll threads completed. Instance is unique!" << endl;

    return 0;
}

/*
OUTPUT:
=== Thread-Safe Singleton (Double-Checked Locking) ===
DatabaseConnection created (Thread-safe)
Executing: SELECT * FROM users
Executing: SELECT * FROM users
Executing: SELECT * FROM users
Executing: SELECT * FROM users
Executing: SELECT * FROM users

All threads completed. Instance is unique!

ADVANTAGES:
- Thread-safe
- Efficient (lock only on first creation)
- Single instance guaranteed

DISADVANTAGES:
- More complex code
- Requires mutex
- Still requires manual memory management
*/
