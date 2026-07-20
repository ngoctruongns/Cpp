#include <iostream>
using namespace std;

// BASIC SINGLETON PATTERN
// This is the simplest form, but NOT thread-safe
class SingleClass {
public:
    // Delete copy constructor and copy assignment
    SingleClass(const SingleClass&) = delete;
    SingleClass& operator=(const SingleClass&) = delete;

    // API to get instance
    static SingleClass* getInstance(void) {
        // If used Meyers' Singleton: dùng biến static của class, hãy dùng biến static bên trong một hàm static.
        // static SingleClass instance

        // Check to create instance
        if (instance_ == nullptr) {
            instance_ = new SingleClass();
        }

        return instance_;
    }

    // Other class methods
    void connect(const std::string str) {
        cout << "Connect to " << str << endl;
    }

    void disconnect(void) {
        cout << "Disconnect" << endl;
    }
private:
    // Pointer for instance
    static SingleClass *instance_;

    // Disable Constructor
    SingleClass() {
        cout << "Constructor class" << endl;
    }

};

// Init for instance pointer in .cpp file to avoid multiple define (or use local static of Meyers' singleton)
SingleClass* SingleClass::instance_ = nullptr;

int main() {
    cout << "=== Basic Singleton Pattern ===" << endl;

    // Get singleton instance
    SingleClass* db1 = SingleClass::getInstance();
    db1->connect("Server1");

    // Get instance again - same object
    SingleClass* db2 = SingleClass::getInstance();
    db2->connect("Server2");

    // Both pointers point to the same object
    cout << "\ndb1 address: " << db1 << endl;
    cout << "db2 address: " << db2 << endl;
    cout << "Are they same? " << (db1 == db2 ? "YES" : "NO") << endl;

    db1->disconnect();

    return 0;
}

/*
OUTPUT:
=== Basic Singleton Pattern ===
SingleClass created
Connected to: Server1
Connected to: Server2

db1 address: 0x...
db2 address: 0x...
Are they same? YES
Disconnected

ADVANTAGES:
- Simple and easy to understand
- Single instance guaranteed

DISADVANTAGES:
- NOT thread-safe (multiple threads can create multiple instances)
- Memory management issues (manual deletion needed)
- Hard to test (tight coupling)
*/
