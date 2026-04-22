/**
 * PHASE 1 - Bài 03: std::unique_ptr — Exclusive Ownership
 *
 * Mục tiêu:
 *  - Thay thế raw pointer + delete bằng unique_ptr
 *  - Hiểu ownership semantics: không copy, chỉ move
 *  - Dùng unique_ptr trong containers, factories
 *  - Custom deleter (quan trọng cho hardware resources)
 *
 * Compile: g++ -std=c++17 03_unique_ptr_basics.cpp -o out
 */

#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <cstdio>

// ─── Sensor hierarchy (giống robot driver) ───────────────────────────────
class Sensor {
public:
    std::string name;
    explicit Sensor(const std::string& n) : name(n) {
        std::cout << "[+] Sensor '" << name << "' created\n";
    }
    virtual ~Sensor() {
        std::cout << "[-] Sensor '" << name << "' destroyed\n";
    }
    virtual double read() const = 0;
};

class LidarSensor : public Sensor {
public:
    explicit LidarSensor(const std::string& n) : Sensor(n) {}
    double read() const override { return 3.14; }  // distance in meters
};

class IMUSensor : public Sensor {
public:
    explicit IMUSensor(const std::string& n) : Sensor(n) {}
    double read() const override { return 9.81; }  // acceleration
};

// ─── Ví dụ 1: Cách cũ (raw pointer) so với unique_ptr ───────────────────
void old_way_raw_pointer() {
    std::cout << "\n=== OLD WAY (raw pointer) ===\n";
    Sensor* s = new LidarSensor("lidar_old");
    // ... nếu throw exception ở đây → memory leak!
    std::cout << "Reading: " << s->read() << "\n";
    delete s;  // phải nhớ delete thủ công
}

void new_way_unique_ptr() {
    std::cout << "\n=== NEW WAY (unique_ptr) ===\n";
    // make_unique preferred (exception-safe, không lộ 'new')
    auto s = std::make_unique<LidarSensor>("lidar_new");
    std::cout << "Reading: " << s->read() << "\n";
    // s tự động bị destroy khi ra khỏi scope
}

// ─── Ví dụ 2: Ownership transfer ─────────────────────────────────────────
std::unique_ptr<Sensor> create_sensor(const std::string& type) {
    // Factory function → trả về unique_ptr (transfer ownership ra ngoài)
    if (type == "lidar") return std::make_unique<LidarSensor>("lidar_main");
    if (type == "imu")   return std::make_unique<IMUSensor>("imu_main");
    return nullptr;
}

void consume_sensor(std::unique_ptr<Sensor> sensor) {
    // Hàm nhận ownership → sensor bị destroy khi hàm kết thúc
    if (sensor) {
        std::cout << "Consuming sensor '" << sensor->name
                  << "', reading=" << sensor->read() << "\n";
    }
}

void ownership_transfer_demo() {
    std::cout << "\n=== Ownership Transfer ===\n";

    auto s = create_sensor("lidar");  // s owns the sensor

    // Không thể copy:
    // auto s2 = s;  // ERROR: use of deleted copy constructor

    // Chuyển ownership bằng std::move:
    consume_sensor(std::move(s));  // s bây giờ là nullptr

    if (!s) std::cout << "s is now nullptr after move\n";
}

// ─── Ví dụ 3: vector<unique_ptr<T>> — polymorphic containers ────────────
void polymorphic_container() {
    std::cout << "\n=== Polymorphic Container ===\n";

    // Lưu nhiều loại sensor khác nhau trong một container
    std::vector<std::unique_ptr<Sensor>> sensors;
    sensors.push_back(std::make_unique<LidarSensor>("front_lidar"));
    sensors.push_back(std::make_unique<LidarSensor>("rear_lidar"));
    sensors.push_back(std::make_unique<IMUSensor>("main_imu"));

    std::cout << "All sensor readings:\n";
    for (const auto& s : sensors) {
        std::cout << "  " << s->name << ": " << s->read() << "\n";
    }
    // Tất cả sensors bị destroy khi vector ra khỏi scope
}

// ─── Ví dụ 4: Custom Deleter — quan trọng cho hardware I/O ───────────────
struct FileDeleter {
    void operator()(FILE* f) const {
        if (f) {
            std::cout << "[FileDeleter] Closing file\n";
            fclose(f);
        }
    }
};

void custom_deleter_demo() {
    std::cout << "\n=== Custom Deleter (FILE*) ===\n";

    // Cách 1: Custom deleter struct
    std::unique_ptr<FILE, FileDeleter> file(fopen("test_output.txt", "w"));
    if (file) {
        fprintf(file.get(), "Hello from unique_ptr managed file!\n");
        std::cout << "Data written to file\n";
    }
    // file tự động fclose khi ra khỏi scope

    // Cách 2: Lambda deleter (thường dùng hơn)
    auto socket_fd = std::unique_ptr<int, std::function<void(int*)>>(
        new int(42),  // Giả lập socket fd
        [](int* fd) {
            std::cout << "[Lambda Deleter] Closing socket fd=" << *fd << "\n";
            // close(*fd);  // Trong thực tế: close() POSIX call
            delete fd;
        }
    );
    std::cout << "Socket fd = " << *socket_fd << "\n";
}

// ─── Ví dụ 5: get(), release(), reset() ──────────────────────────────────
void ptr_operations_demo() {
    std::cout << "\n=== unique_ptr Operations ===\n";

    auto s = std::make_unique<LidarSensor>("ops_lidar");

    // get(): truy cập raw pointer (không chuyển ownership)
    Sensor* raw = s.get();
    std::cout << "raw ptr address: " << raw << "\n";
    std::cout << "unique_ptr still owns: " << (s ? "yes" : "no") << "\n";

    // reset(): xóa object hiện tại, có thể assign cái mới
    s.reset(new LidarSensor("new_lidar"));  // old bị destroy
    std::cout << "After reset: " << s->name << "\n";

    // release(): trả ra raw pointer, từ bỏ ownership (NGUY HIỂM)
    Sensor* released = s.release();
    std::cout << "After release, s is null: " << (!s ? "yes" : "no") << "\n";
    delete released;  // phải delete thủ công!
}

int main() {
    old_way_raw_pointer();
    new_way_unique_ptr();
    ownership_transfer_demo();
    polymorphic_container();
    custom_deleter_demo();
    ptr_operations_demo();

    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Viết class `RobotArm` chứa `unique_ptr<Sensor>` cho end-effector.
 *    RobotArm không copyable nhưng movable. Test move semantics.
 *
 * 2. Viết factory function `make_sensor(std::string type, std::string name)`
 *    trả về unique_ptr<Sensor>, ném exception nếu type không hợp lệ.
 *    Test rằng khi exception xảy ra, không có memory leak.
 *
 * 3. Thêm camera sensor, sonar sensor vào polymorphic_container().
 *    Sau đó viết hàm tính average reading của tất cả sensors.
 *
 * 4. [Nâng cao] Dùng unique_ptr với custom deleter để wrap GPIO pin
 *    trên Linux (/sys/class/gpio/export). Deleter phải unexport pin.
 *
 * ═══ LIÊN KẾT ROS2 ═════════════════════════════════════════════════════════
 *
 * Trong ROS2:
 *   // Plugin loader trả về unique_ptr
 *   std::unique_ptr<nav2_core::GlobalPlanner> planner =
 *       plugin_loader_.createUniqueInstance(plugin_name);
 *
 *   // Node factory dùng unique_ptr
 *   auto node = std::make_unique<MyNode>(options);
 *   rclcpp::spin(std::move(node)); // ownership transfer
 */
