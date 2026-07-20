#include <iostream>
#include "Singleton.hpp"
using namespace std;

// Example 1: Simple Logger using template Singleton
class Logger : public Singleton<Logger> {
private:
    int logCount;

    Logger() : logCount(0) {
        cout << "[Logger] Initialized" << endl;
    }
    friend class Singleton<Logger>;

public:
    void log(const string& message) {
        logCount++;
        cout << "[LOG #" << logCount << "] " << message << endl;
    }

    int getLogCount() const { return logCount; }
};


// Example 2: Settings using template Singleton
class Settings : public Singleton<Settings> {
private:
    string theme;
    int fontSize;

    Settings() : theme("light"), fontSize(12) {
        cout << "[Settings] Initialized" << endl;
    }
    friend class Singleton<Settings>;

public:
    void setTheme(const string& t) { theme = t; }
    string getTheme() const { return theme; }

    void setFontSize(int size) { fontSize = size; }
    int getFontSize() const { return fontSize; }

    void printSettings() {
        cout << "Theme: " << theme << ", Font Size: " << fontSize << "pt" << endl;
    }
};


// Example 3: Database using template Singleton
class Database : public Singleton<Database> {
private:
    int connectionCount;

    Database() : connectionCount(0) {
        cout << "[Database] Connection initialized" << endl;
    }
    friend class Singleton<Database>;

public:
    bool connect() {
        connectionCount++;
        cout << "Connected to database (connection #" << connectionCount << ")" << endl;
        return true;
    }
};


int main() {
    cout << "=== Template-based Singleton Pattern ===" << endl << endl;

    // Use Logger singleton
    cout << "Using Logger:" << endl;
    Logger::getInstance().log("Application started");
    Logger::getInstance().log("Loading configuration");
    Logger::getInstance().log("Ready");
    cout << "Total logs: " << Logger::getInstance().getLogCount() << endl;

    cout << endl;

    // Use Settings singleton
    cout << "Using Settings:" << endl;
    Settings::getInstance().setTheme("dark");
    Settings::getInstance().setFontSize(14);
    Settings::getInstance().printSettings();

    cout << endl;

    // Use Database singleton
    cout << "Using Database:" << endl;
    Database::getInstance().connect();
    Database::getInstance().connect();

    cout << "\n✓ All singletons are unique instances!" << endl;

    return 0;
}

/*
OUTPUT:
=== Template-based Singleton Pattern ===

Using Logger:
[Logger] Initialized
[LOG #1] Application started
[LOG #2] Loading configuration
[LOG #3] Ready
Total logs: 3

Using Settings:
[Settings] Initialized
Theme: dark, Font Size: 14pt

Using Database:
[Database] Connection initialized
Connected to database (connection #1)
Connected to database (connection #2)

✓ All singletons are unique instances!

ADVANTAGES OF TEMPLATE-BASED SINGLETON:
- Reusable for any class
- Type-safe
- No code duplication
- Easy to apply to existing classes
- CRTP (Curiously Recurring Template Pattern)
*/
