# Singleton Pattern in C++

Hướng dẫn toàn diện về mẫu **Singleton** trong C++. Singleton là một design pattern giúp đảm bảo rằng một lớp chỉ có duy nhất một instance (đối tượng) trong toàn bộ ứng dụng.

## 📚 Nội dung

### 1️⃣ [01_basic_singleton.cpp](01_basic_singleton.cpp)
**Singleton cơ bản** - Phiên bản đơn giản nhất

```cpp
class DatabaseConnection {
private:
    static DatabaseConnection* instance;
    DatabaseConnection() { }  // Private constructor

public:
    static DatabaseConnection* getInstance() {
        if (instance == nullptr) {
            instance = new DatabaseConnection();
        }
        return instance;
    }
};
```

**Đặc điểm:**
- ✅ Đơn giản, dễ hiểu
- ❌ Không an toàn với thread (thread-unsafe)
- ❌ Cần quản lý memory thủ công

**Khi nào dùng:** Chỉ cho ứng dụng single-threaded hoặc đơn giản

---

### 2️⃣ [02_thread_safe_singleton.cpp](02_thread_safe_singleton.cpp)
**Singleton an toàn với thread** - Double-Checked Locking Pattern

```cpp
class DatabaseConnection {
private:
    static DatabaseConnection* instance;
    static mutex createMutex;

public:
    static DatabaseConnection* getInstance() {
        if (instance == nullptr) {           // Check 1 (no lock)
            lock_guard<mutex> lock(createMutex);
            if (instance == nullptr) {       // Check 2 (with lock)
                instance = new DatabaseConnection();
            }
        }
        return instance;
    }
};
```

**Đặc điểm:**
- ✅ Thread-safe (an toàn với đa luồng)
- ✅ Hiệu suất tốt (lock chỉ khi cần)
- ❌ Code phức tạp hơn
- ❌ Vẫn cần quản lý memory thủ công

**Khi nào dùng:** Ứng dụng multi-threaded cần control rõ ràng

---

### 3️⃣ [03_meyers_singleton.cpp](03_meyers_singleton.cpp)
**Meyers Singleton** - ⭐ PHƯƠNG PHÁP KHUYẾN NGHỊ

```cpp
class Logger {
private:
    Logger() { }  // Private constructor

public:
    static Logger& getInstance() {
        static Logger instance;  // Thread-safe by compiler guarantee (C++11+)
        return instance;
    }
};
```

**Đặc điểm:**
- ✅ Thread-safe tự động (C++11+)
- ✅ Code siêu đơn giản
- ✅ Tự động cleanup (gọi destructor khi kết thúc chương trình)
- ✅ Không có memory leak
- ✅ Trả về reference thay vì pointer
- ❌ Không có (đây là cách tốt nhất!)

**Cách hoạt động:**
- Biến static cục bộ được khởi tạo chỉ một lần
- Compiler tự động chèn code đồng bộ (synchronization)
- Đây là lựa chọn tốt nhất cho C++ hiện đại

**Khi nào dùng:** Gần như tất cả các trường hợp trong C++11 trở lên

---

### 4️⃣ [04_shared_singleton.cpp](04_shared_singleton.cpp)
**Singleton với shared_ptr** - Hiện đại hơn

```cpp
class ConfigManager {
private:
    static shared_ptr<ConfigManager> instance;

public:
    static shared_ptr<ConfigManager> getInstance() {
        static once_flag initFlag;
        call_once(initFlag, []() {
            instance = make_shared<ConfigManager>();
        });
        return instance;
    }
};
```

**Đặc điểm:**
- ✅ Thread-safe
- ✅ Reference counting (tự động cleanup)
- ✅ Exception-safe
- ✅ Có thể có nhiều tham chiếu
- ⚠️ Có overhead từ reference counting

**Khi nào dùng:** Khi cần đối tượng có thể có nhiều owner hoặc cần release rõ ràng

---

### 5️⃣ [05_practical_example.cpp](05_practical_example.cpp)
**Ví dụ thực tế** - Sử dụng Singleton trong ứng dụng thực

Ví dụ lớp `UserService` sử dụng hai singleton:
- `AppConfig` - quản lý cấu hình ứng dụng
- `DatabasePool` - quản lý connection pool

```cpp
class UserService {
    void createUser(const string& username) {
        AppConfig& config = AppConfig::getInstance();
        DatabasePool& dbPool = DatabasePool::getInstance();

        if (dbPool.getConnection()) {
            cout << "Creating user: " << username << endl;
            dbPool.releaseConnection();
        }
    }
};
```

---

### 6️⃣ [06_comparison_and_antipatterns.cpp](06_comparison_and_antipatterns.cpp)
**So sánh các cách và Anti-patterns**

Thảo luận:
- ❌ **Anti-pattern 1:** Global variable (tệ!)
- ❌ **Anti-pattern 2:** Function with static variable (kém chuyên nghiệp)
- ✅ **Good:** Proper Singleton với Meyers pattern
- ✅ **Better:** Dependency Injection (dễ test hơn)

**Cảnh báo:**
```cpp
// ❌ KHÔNG LÀM CÁI NÀY
class BadConfig {
public:
    string setting = "default";
};
BadConfig globalConfig;  // Global variable - tệ!

// ✅ LÀM CÁI NÀY
class GoodConfig {
private:
    GoodConfig() { }
public:
    static GoodConfig& getInstance() {
        static GoodConfig instance;
        return instance;
    }
};
```

---

### 7️⃣ [07_template_singleton.cpp](07_template_singleton.cpp) + [Singleton.hpp](Singleton.hpp)
**Template-based Singleton** - Dễ reuse cho bất kỳ lớp nào

Sử dụng CRTP (Curiously Recurring Template Pattern) để tạo Singleton tái sử dụng:

```cpp
// File: Singleton.hpp
template<typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

public:
    static T& getInstance() {
        static T instance;
        return instance;
    }
};

// Sử dụng:
class Logger : public Singleton<Logger> {
private:
    Logger() { }
    friend class Singleton<Logger>;
public:
    void log(const string& msg) { }
};

Logger::getInstance().log("test");
```

**Đặc điểm:**
- ✅ Tái sử dụng cho nhiều lớp
- ✅ Type-safe
- ✅ Không code duplication
- ✅ CRTP pattern
- ✅ Đơn giản và sạch

**Khi nào dùng:** Khi có nhiều lớp cần làm Singleton

---

## 🚀 Cách sử dụng

### Biên dịch và chạy tất cả ví dụ:

```bash
cd /home/tvn/code_ws/Cpp/Pattern/Singleton
mkdir build && cd build
cmake ..
make

# Chạy riêng từng ví dụ
./01_basic_singleton
./02_thread_safe_singleton
./03_meyers_singleton
./04_shared_singleton
./05_practical_example
./06_comparison_and_antipatterns

# Hoặc chạy tất cả
make run_all
```

---

## 📊 So sánh nhanh

| Feature | Basic | Double-Check | Meyers | Shared_ptr | Template |
|---------|-------|--------------|--------|------------|----------|
| Thread-safe | ❌ | ✅ | ✅ | ✅ | ✅ |
| Code complexity | ⭐ | ⭐⭐⭐ | ⭐ | ⭐⭐ | ⭐⭐ |
| Memory management | Manual | Manual | Auto | Auto (RC) | Auto |
| Reusable | ❌ | ❌ | ❌ | ❌ | ✅ |
| C++ version | Any | C++11+ | C++11+ | C++11+ | C++11+ |
| Recommended | ❌ | ⚠️ | ✅✅✅ | ✅ | ✅✅ |

---

## 🎯 Khi nào dùng Singleton

### ✅ Hợp lý
- Configuration managers (quản lý cấu hình)
- Logger / Logging systems
- Database connection pools
- Thread pools
- Resource managers
- Application state

### ❌ Không hợp lý
- Business logic classes
- Services có nhiều instance
- Classes cần test độc lập
- Khi muốn nhiều variant của resource
- Thay thế cho design chính xác

---

## 💡 Best Practices

1. **Luôn dùng Meyers Singleton** cho C++11+
   ```cpp
   static MyClass& getInstance() {
       static MyClass instance;
       return instance;
   }
   ```

2. **Xoá copy constructor và assignment operator**
   ```cpp
   MyClass(const MyClass&) = delete;
   MyClass& operator=(const MyClass&) = delete;
   ```

3. **Giữ singleton tối giản** - chỉ chứa state cần dùng chung

4. **Cân nhắc Dependency Injection** để dễ test hơn
   ```cpp
   class Service {
       MyConfig& config;
       Service(MyConfig& cfg) : config(cfg) { }
   };
   ```

5. **Tài liệu hóa tại sao cần Singleton** - không phải mọi trường hợp đều cần

6. **Tránh Singleton trong unit tests** - khó mock

---

## 📖 Tài liệu thêm

- [C++ Singleton Pattern - Wikipedia](https://en.wikipedia.org/wiki/Singleton_pattern)
- [Meyers Singleton in C++11](https://en.cppreference.com/w/cpp/language/static)
- [Design Patterns: Elements of Reusable Object-Oriented Software](https://en.wikipedia.org/wiki/Design_Patterns)

---

## 🔗 Liên quan

Xem thêm các Design Pattern khác tại [Pattern/](../)
