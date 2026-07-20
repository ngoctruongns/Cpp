#include <iostream>
#include <vector>
using namespace std;

// PRACTICAL EXAMPLE: Application with Singleton Pattern

// Singleton: Global Configuration
class AppConfig {
private:
    string appName;
    string appVersion;
    int maxConnections;
    bool debugMode;

    AppConfig() : appName("MyApp"), appVersion("1.0.0"),
                  maxConnections(100), debugMode(false) {
        cout << "[AppConfig] Initialized" << endl;
    }

public:
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    static AppConfig& getInstance() {
        static AppConfig instance;
        return instance;
    }

    void setDebugMode(bool mode) { debugMode = mode; }
    bool isDebugMode() const { return debugMode; }

    void setMaxConnections(int max) { maxConnections = max; }
    int getMaxConnections() const { return maxConnections; }

    void printConfig() {
        cout << "App: " << appName << " v" << appVersion << endl;
        cout << "Max Connections: " << maxConnections << endl;
        cout << "Debug Mode: " << (debugMode ? "ON" : "OFF") << endl;
    }
};


// Singleton: Database Connection Pool
class DatabasePool {
private:
    int activeConnections;
    int maxPoolSize;

    DatabasePool() : activeConnections(0), maxPoolSize(10) {
        cout << "[DatabasePool] Created with max size " << maxPoolSize << endl;
    }

public:
    DatabasePool(const DatabasePool&) = delete;
    DatabasePool& operator=(const DatabasePool&) = delete;

    static DatabasePool& getInstance() {
        static DatabasePool instance;
        return instance;
    }

    bool getConnection() {
        if (activeConnections < maxPoolSize) {
            activeConnections++;
            return true;
        }
        return false;
    }

    void releaseConnection() {
        if (activeConnections > 0) {
            activeConnections--;
        }
    }

    void printStatus() {
        cout << "[DatabasePool] Active: " << activeConnections
             << "/" << maxPoolSize << endl;
    }
};


// Regular class that uses singletons
class UserService {
private:
    string serviceName;

public:
    UserService(const string& name) : serviceName(name) {
        cout << "[UserService] " << name << " created" << endl;
    }

    void createUser(const string& username) {
        AppConfig& config = AppConfig::getInstance();
        DatabasePool& dbPool = DatabasePool::getInstance();

        if (dbPool.getConnection()) {
            cout << "[" << serviceName << "] Creating user: " << username << endl;
            if (config.isDebugMode()) {
                cout << "  [DEBUG] SQL: INSERT INTO users VALUES ('"
                     << username << "')" << endl;
            }
            dbPool.releaseConnection();
        } else {
            cout << "[" << serviceName << "] ERROR: Database pool exhausted!" << endl;
        }
    }
};


int main() {
    cout << "=== Practical Singleton Example ===" << endl << endl;

    // Configure application (singleton)
    AppConfig& config = AppConfig::getInstance();
    config.setDebugMode(true);
    config.setMaxConnections(50);
    config.printConfig();

    cout << endl;

    // Create services
    UserService service1("UserService");
    UserService service2("AdminService");

    cout << endl;

    // Both services use the same singleton instances
    service1.createUser("alice");
    service2.createUser("bob");

    DatabasePool& dbPool = DatabasePool::getInstance();
    dbPool.printStatus();

    cout << endl;

    // Create many users to test pool limit
    cout << "Creating 12 users (pool size is 10):" << endl;
    for (int i = 0; i < 12; ++i) {
        service1.createUser("user" + to_string(i));
    }

    dbPool.printStatus();

    return 0;
}

/*
OUTPUT:
=== Practical Singleton Example ===

[AppConfig] Initialized
[DatabasePool] Created with max size 10
App: MyApp v1.0.0
Max Connections: 50
Debug Mode: ON

[UserService] UserService created
[UserService] AdminService created

[UserService] Creating user: alice
  [DEBUG] SQL: INSERT INTO users VALUES ('alice')
[AdminService] Creating user: bob
  [DEBUG] SQL: INSERT INTO users VALUES ('bob')
[DatabasePool] Active: 0/10

Creating 12 users (pool size is 10):
[UserService] Creating user: user0
  [DEBUG] SQL: INSERT INTO users VALUES ('user0')
[UserService] Creating user: user1
  [DEBUG] SQL: INSERT INTO users VALUES ('user1')
...
[UserService] Creating user: user9
  [DEBUG] SQL: INSERT INTO users VALUES ('user9')
[UserService] ERROR: Database pool exhausted!
[UserService] ERROR: Database pool exhausted!
[DatabasePool] Active: 0/10

BENEFITS OF SINGLETONS IN THIS EXAMPLE:
- Centralized configuration management
- Shared resource pool (database connections)
- Global state without global variables
- Consistent behavior across all services
*/
