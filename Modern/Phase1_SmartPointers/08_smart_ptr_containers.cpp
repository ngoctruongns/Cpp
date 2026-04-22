/**
 * PHASE 1 - Bài 08: Smart Pointers in Containers & Polymorphism
 *
 * Mục tiêu:
 *  - Lưu trữ polymorphic objects trong vector<unique_ptr<Base>>
 *  - Chuyển giữa unique_ptr và shared_ptr
 *  - Tránh slicing với smart pointers
 *  - Factory pattern + type-safe sensor registry
 *
 * Compile: g++ -std=c++17 08_smart_ptr_containers.cpp -o out
 */

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <functional>

// ─── Sensor hierarchy ────────────────────────────────────────────────────
class Sensor {
public:
    std::string name;
    int         id;

    Sensor(std::string n, int i) : name(std::move(n)), id(i) {
        std::cout << "[+] Sensor '" << name << "' id=" << id << "\n";
    }
    virtual ~Sensor() {
        std::cout << "[-] Sensor '" << name << "' id=" << id << "\n";
    }

    virtual double  read()        const = 0;
    virtual std::string type_name() const = 0;
    virtual void    print_info()  const {
        std::cout << "[" << type_name() << "] '" << name
                  << "' value=" << read() << "\n";
    }
};

class LidarSensor : public Sensor {
    double range_;
public:
    LidarSensor(const std::string& n, int id, double range)
        : Sensor(n, id), range_(range) {}
    double read() const override { return range_; }
    std::string type_name() const override { return "Lidar"; }
};

class IMUSensor : public Sensor {
    double accel_;
public:
    IMUSensor(const std::string& n, int id, double accel)
        : Sensor(n, id), accel_(accel) {}
    double read() const override { return accel_; }
    std::string type_name() const override { return "IMU"; }
};

class CameraSensor : public Sensor {
    int resolution_;
public:
    CameraSensor(const std::string& n, int id, int res)
        : Sensor(n, id), resolution_(res) {}
    double read() const override { return static_cast<double>(resolution_); }
    std::string type_name() const override { return "Camera"; }
};

// ─── Ví dụ 1: Object Slicing Problem ─────────────────────────────────────
void demo_slicing_problem() {
    std::cout << "\n=== Object Slicing Problem ===\n";

    // WRONG: vector<Sensor> → slicing! Chỉ lưu được base part
    // vector<Sensor> sensors;
    // sensors.push_back(LidarSensor("lidar", 1, 3.14));  // sliced!

    // CORRECT: vector<unique_ptr<Sensor>> → polymorphism đúng
    std::vector<std::unique_ptr<Sensor>> sensors;
    sensors.push_back(std::make_unique<LidarSensor>("lidar_front", 1, 3.14));
    sensors.push_back(std::make_unique<IMUSensor>("imu_main", 2, 9.81));
    sensors.push_back(std::make_unique<CameraSensor>("cam_rgb", 3, 1920));

    std::cout << "Reading all sensors:\n";
    for (const auto& s : sensors) {
        s->print_info();  // virtual dispatch — đúng type!
    }
}

// ─── Ví dụ 2: Factory function + registry ────────────────────────────────
class SensorRegistry {
    std::unordered_map<int, std::shared_ptr<Sensor>> sensors_;
    int next_id_ = 1;

public:
    template<typename T, typename... Args>
    std::shared_ptr<T> create_sensor(Args&&... args) {
        auto s = std::make_shared<T>(std::forward<Args>(args)..., next_id_++);
        sensors_[s->id] = s;  // store as shared_ptr<Sensor> (covariant)
        return s;
    }

    std::shared_ptr<Sensor> get(int id) {
        auto it = sensors_.find(id);
        return (it != sensors_.end()) ? it->second : nullptr;
    }

    void print_all() const {
        std::cout << "=== Registry (" << sensors_.size() << " sensors) ===\n";
        for (const auto& [id, s] : sensors_) {
            std::cout << "  id=" << id << " ";
            s->print_info();
        }
    }

    size_t size() const { return sensors_.size(); }
};

void demo_registry() {
    std::cout << "\n=== Sensor Registry with shared_ptr ===\n";

    SensorRegistry reg;
    auto lidar = reg.create_sensor<LidarSensor>("lidar_front", 5.0);
    auto imu   = reg.create_sensor<IMUSensor>("imu_rear", 9.81);
    auto cam   = reg.create_sensor<CameraSensor>("cam_left", 1280);

    reg.print_all();

    // Người dùng giữ shared_ptr riêng → sensor sống đến khi cần
    std::cout << "lidar use_count=" << lidar.use_count() << "\n";  // 2 (reg + lidar)
}

// ─── Ví dụ 3: Move unique_ptr giữa containers ────────────────────────────
void demo_move_between_containers() {
    std::cout << "\n=== Move unique_ptr between containers ===\n";

    std::vector<std::unique_ptr<Sensor>> pool;
    pool.push_back(std::make_unique<LidarSensor>("l1", 1, 1.0));
    pool.push_back(std::make_unique<LidarSensor>("l2", 2, 2.0));
    pool.push_back(std::make_unique<IMUSensor>("i1", 3, 9.81));

    // Tách sensors theo loại
    std::vector<std::unique_ptr<Sensor>> lidars, imus;
    for (auto& s : pool) {
        if (s->type_name() == "Lidar") {
            lidars.push_back(std::move(s));  // move ownership
        } else {
            imus.push_back(std::move(s));
        }
    }
    // pool vẫn có 3 phần tử nhưng tất cả là nullptr (moved-from)
    std::cout << "Lidars: " << lidars.size() << ", IMUs: " << imus.size() << "\n";

    // Dọn nullptr khỏi pool
    pool.erase(
        std::remove_if(pool.begin(), pool.end(),
                       [](const auto& p){ return p == nullptr; }),
        pool.end()
    );
    std::cout << "Pool after move: " << pool.size() << " (should be 0)\n";
}

// ─── Ví dụ 4: Chuyển unique_ptr → shared_ptr ──────────────────────────────
void demo_unique_to_shared() {
    std::cout << "\n=== unique_ptr → shared_ptr conversion ===\n";

    // unique_ptr → shared_ptr: OK (implicit conversion)
    std::unique_ptr<LidarSensor> up = std::make_unique<LidarSensor>("lidar", 1, 3.5);
    std::shared_ptr<Sensor> sp = std::move(up);  // up becomes null
    std::cout << "up is null: " << (up == nullptr) << "\n";
    std::cout << "sp use_count: " << sp.use_count() << "\n";

    // shared_ptr → unique_ptr: KHÔNG thể (shared ownership trái với unique)
    // std::unique_ptr<Sensor> up2 = std::move(sp);  // compile error
}

int main() {
    demo_slicing_problem();
    demo_registry();
    demo_move_between_containers();
    demo_unique_to_shared();
    std::cout << "\n=== All sensors destroyed ===\n";
    return 0;
}
