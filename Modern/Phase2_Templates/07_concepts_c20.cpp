/**
 * PHASE 2 - Bài 07: C++20 Concepts
 *
 * Mục tiêu:
 *  - Định nghĩa và dùng concept để constrain templates
 *  - Thay thế SFINAE bằng requires clause
 *  - Viết concepts cho robot/ROS2 interfaces
 *
 * Compile: g++ -std=c++20 07_concepts_c20.cpp -o out
 */

#include <iostream>
#include <concepts>
#include <string>
#include <vector>
#include <type_traits>

// ─── Ví dụ 1: Built-in concepts từ <concepts> ────────────────────────────
template<std::integral T>
T gcd(T a, T b) {
    while (b) { a %= b; std::swap(a, b); }
    return a;
}

template<std::floating_point T>
T lerp(T a, T b, T t) {
    return a + t * (b - a);  // linear interpolation
}

// ─── Ví dụ 2: Custom concept — Sensor interface ───────────────────────────
// Bất kỳ type nào có method read() trả về double đều là Sensor
template<typename T>
concept Sensor = requires(T sensor) {
    { sensor.read() } -> std::convertible_to<double>;
    { sensor.name() } -> std::convertible_to<std::string>;
};

// Bất kỳ type nào có method write(double) đều là Actuator
template<typename T>
concept Actuator = requires(T actuator, double val) {
    { actuator.write(val) } -> std::same_as<void>;
    { actuator.name() } -> std::convertible_to<std::string>;
};

// Combination concept
template<typename T>
concept SensorActuator = Sensor<T> && Actuator<T>;

// ─── Ví dụ 3: Implementations thỏa mãn concepts ──────────────────────────
struct LidarSensor {
    std::string name() const { return "lidar"; }
    double read() const { return 1.5; }  // distance in meters
};

struct MotorActuator {
    std::string name() const { return "motor"; }
    void write(double speed) { speed_ = speed; }
    double speed() const { return speed_; }
private:
    double speed_ = 0.0;
};

struct SonarUnit {  // Thỏa mãn cả Sensor và Actuator
    std::string name() const { return "sonar"; }
    double read() const { return 0.8; }  // distance
    void write(double freq) { freq_ = freq; }  // ping frequency
private:
    double freq_ = 40000.0;
};

// ─── Ví dụ 4: Generic functions với concepts ─────────────────────────────
template<Sensor S>
void calibrate(S& sensor) {
    std::cout << "Calibrating " << sensor.name() << ", reading=" << sensor.read() << "\n";
}

template<Actuator A>
void home_position(A& actuator) {
    actuator.write(0.0);
    std::cout << "Homing " << actuator.name() << "\n";
}

template<SensorActuator SA>
void diagnostic(SA& device) {
    std::cout << "Diagnostic " << device.name()
              << ": read=" << device.read() << "\n";
    device.write(0.0);
}

// ─── Ví dụ 5: Concept với requires expression ────────────────────────────
template<typename T>
concept Serializable = requires(T t) {
    // T phải có serialize() trả về vector<uint8_t>
    { t.serialize() } -> std::same_as<std::vector<uint8_t>>;
    // T phải có static deserialize(bytes) trả về T
    { T::deserialize(std::declval<std::vector<uint8_t>>()) } -> std::same_as<T>;
    // T phải có size() trả về size_t
    { t.size() } -> std::convertible_to<std::size_t>;
};

// ─── Ví dụ 6: RobotMessage concept ───────────────────────────────────────
template<typename T>
concept RobotMessage = requires(T msg) {
    // Phải có header với timestamp
    { msg.header.stamp } -> std::convertible_to<uint64_t>;
    { msg.header.frame_id } -> std::convertible_to<std::string>;
    // Phải có serialize
    { msg.serialize() } -> std::same_as<std::vector<uint8_t>>;
};

struct LaserScan {
    struct { uint64_t stamp; std::string frame_id; } header;
    std::vector<float> ranges;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> bytes;
        // Simplified serialization
        bytes.push_back(static_cast<uint8_t>(ranges.size()));
        return bytes;
    }
};

template<RobotMessage Msg>
void publish(const Msg& msg) {
    std::cout << "Publishing message from frame '"
              << msg.header.frame_id << "' at t="
              << msg.header.stamp << "\n";
    auto bytes = msg.serialize();
    std::cout << "Serialized to " << bytes.size() << " bytes\n";
}

// ─── Ví dụ 7: requires clause trực tiếp ──────────────────────────────────
// Thay vì tạo concept riêng, đặt requires trực tiếp
template<typename T>
requires std::is_arithmetic_v<T> && (sizeof(T) >= 4)
T clamp(T val, T lo, T hi) {
    return std::max(lo, std::min(val, hi));
}

// ─── Ví dụ 8: Concept cho callback / callable ────────────────────────────
template<typename F, typename... Args>
concept Callback = std::invocable<F, Args...>;

template<Callback<double, std::string> F>
void on_sensor_data(double value, const std::string& source, F&& cb) {
    std::cout << "Received data from " << source << ": " << value << "\n";
    std::forward<F>(cb)(value, source);
}

int main() {
    // === Ví dụ 1 ===
    std::cout << "=== Built-in Concepts ===\n";
    std::cout << "gcd(12, 8) = " << gcd(12, 8) << "\n";
    std::cout << "lerp(0.0, 10.0, 0.3) = " << lerp(0.0, 10.0, 0.3) << "\n";
    // gcd(1.5, 2.5);  // Compile error: 'double' doesn't satisfy integral

    // === Ví dụ 2-4: Custom concepts ===
    std::cout << "\n=== Custom Sensor/Actuator Concepts ===\n";
    LidarSensor lidar;
    MotorActuator motor;
    SonarUnit sonar;

    calibrate(lidar);
    home_position(motor);
    diagnostic(sonar);

    // calibrate(motor);  // Compile error: MotorActuator doesn't satisfy Sensor

    // === Ví dụ 6: RobotMessage ===
    std::cout << "\n=== RobotMessage Concept ===\n";
    LaserScan scan;
    scan.header.stamp = 1234567890;
    scan.header.frame_id = "base_laser";
    scan.ranges = {1.2f, 0.8f, 2.1f, 1.5f};

    publish(scan);

    // === Ví dụ 7: requires clause ===
    std::cout << "\n=== Clamp with requires ===\n";
    std::cout << "clamp(15.0, 0.0, 10.0) = " << clamp(15.0, 0.0, 10.0) << "\n";
    // clamp('a', '0', '9');  // Error: char is only 1 byte

    // === Ví dụ 8: Callback concept ===
    std::cout << "\n=== Callback Concept ===\n";
    on_sensor_data(3.14, "ultrasonic",
        [](double v, const std::string& src) {
            std::cout << "  Callback: processed " << v << " from " << src << "\n";
        }
    );

    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Viết concept `Printable<T>` — T có thể in ra std::ostream bằng <<.
 *    Viết `print_if_printable(T val)` sử dụng concept này.
 *
 * 2. Viết concept `Container<T>` thỏa mãn:
 *    - có begin() và end()
 *    - có size()
 *    - có value_type
 *    Viết `stats(container)` tính min/max/mean cho Container<T>
 *    where T satisfies std::is_arithmetic.
 *
 * 3. Viết concept `ROS2Node<T>` đặc trưng:
 *    - có get_name() trả về string
 *    - có spin() không trả về gì
 *    - có create_publisher<M>(topic, qos) trả về shared_ptr
 *    (Chỉ định nghĩa concept, không cần implement đầy đủ)
 *
 * ═══ LƯU Ý QUAN TRỌNG ══════════════════════════════════════════════════════
 *
 * Concepts giúp:
 *   1. Error messages rõ ràng hơn SFINAE (đọc được!)
 *   2. Code dễ đọc, tự document
 *   3. Compiler bắt lỗi sớm hơn (constraint violation vs substitution failure)
 *
 * Trong ROS2:
 *   template<typename MessageT>
 *   requires rosidl_generator_traits::is_message<MessageT>::value
 *   class Publisher { ... };
 */
