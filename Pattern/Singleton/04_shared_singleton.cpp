#include <iostream>
#include <memory>
#include <mutex>
using namespace std;

// SINGLETON WITH SHARED_PTR (Modern C++11/14+)
// Combines benefits of Meyers Singleton with explicit memory management

class ConfigManager {
private:
    static shared_ptr<ConfigManager> instance;

    // Private constructor
    ConfigManager() {
        cout << "ConfigManager initialized" << endl;
    }

public:
    // Delete copy operations
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Get singleton instance
    static shared_ptr<ConfigManager> getInstance() {
        static once_flag initFlag;
        call_once(initFlag, []() {
            instance = shared_ptr<ConfigManager>(new ConfigManager());
        });
        return instance;
    }

    void setSetting(const string& key, const string& value) {
        cout << "Setting [" << key << "] = " << value << endl;
    }

    void printSettings() {
        cout << "ConfigManager use_count: " << instance.use_count() << endl;
    }
};

shared_ptr<ConfigManager> ConfigManager::instance = nullptr;


int main() {
    cout << "=== Singleton with shared_ptr ===" << endl;

    {
        auto config1 = ConfigManager::getInstance();
        config1->setSetting("database", "localhost");
        config1->printSettings();

        {
            auto config2 = ConfigManager::getInstance();
            config2->setSetting("port", "5432");
            config2->printSettings();
        }

        cout << "After config2 goes out of scope:" << endl;
        config1->printSettings();
    }

    cout << "\nAll shared_ptr instances destroyed!" << endl;

    return 0;
}

/*
OUTPUT:
=== Singleton with shared_ptr ===
ConfigManager initialized
Setting [database] = localhost
ConfigManager use_count: 1
Setting [port] = 5432
ConfigManager use_count: 2
After config2 goes out of scope:
ConfigManager use_count: 1

All shared_ptr instances destroyed!

ADVANTAGES:
- Automatic memory management
- Reference counting
- Exception-safe
- Multiple references to same instance
- No manual cleanup

DISADVANTAGES:
- Slight overhead from reference counting
- More complex syntax
*/
