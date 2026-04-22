/**
 * PHASE 3 - Bài 10: Multi-Sensor Data Fusion (Bài tổng hợp)
 *
 * Kết hợp toàn bộ Phase 3:
 *  - std::thread: dedicated threads cho mỗi sensor
 *  - mutex + lock_guard: protect shared state
 *  - condition_variable: synchronize fusion với sensor data
 *  - atomic: counters, stop flags
 *  - Thread pool: fusion workers
 *  - jthread (C++20): RAII threads with stop token
 *
 * Scenario: LiDAR + IMU + Camera → Sensor Fusion Node
 *
 * Compile: g++ -std=c++17 -pthread 10_sensor_fusion_sim.cpp -o out
 * (GCC 10+ with -std=c++20: replace JThread/StopToken with JThread/StopToken)
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <vector>
#include <chrono>
#include <string>
#include <functional>
#include <optional>
#include <random>
#include <memory>

// ── C++17 compatible jthread/stop_token ─────────────────────────────────
struct StopToken {
    std::shared_ptr<std::atomic<bool>> flag_;
    explicit StopToken(std::shared_ptr<std::atomic<bool>> f) : flag_(std::move(f)) {}
    bool stop_requested() const { return flag_->load(std::memory_order_relaxed); }
};

class JThread {
    std::thread                        t_;
    std::shared_ptr<std::atomic<bool>> flag_;
public:
    JThread() : flag_(std::make_shared<std::atomic<bool>>(false)) {}
    template<typename F>
    explicit JThread(F&& fn) : flag_(std::make_shared<std::atomic<bool>>(false)) {
        auto f = flag_;
        t_ = std::thread([fn=std::forward<F>(fn), f]() mutable { fn(StopToken{f}); });
    }
    JThread(JThread&&) = default;
    JThread& operator=(JThread&&) = default;
    JThread(const JThread&) = delete;
    JThread& operator=(const JThread&) = delete;
    ~JThread() { if (t_.joinable()) { flag_->store(true); t_.join(); } }
    void request_stop() { flag_->store(true, std::memory_order_relaxed); }
    void join() { if (t_.joinable()) t_.join(); }
};
// ────────────────────────────────────────────────────────────────────────

using namespace std::chrono_literals;

// ══════════════════════════════════════════════════════════════════════════
// PART 1: Data Types
// ══════════════════════════════════════════════════════════════════════════

using TimeNs = uint64_t;

struct LidarData {
    TimeNs    stamp;
    float     range;       // meters
    float     angle;       // radians
};

struct ImuData {
    TimeNs    stamp;
    double    ax, ay, az;  // m/s²
    double    gx, gy, gz;  // rad/s
};

struct CameraData {
    TimeNs    stamp;
    int       width, height;
    int       detected_objects;
};

struct FusedEstimate {
    TimeNs    stamp;
    double    x, y;        // estimated position
    double    vx, vy;      // estimated velocity
    int       confidence;  // 0–100
    std::string source;
};

// ══════════════════════════════════════════════════════════════════════════
// PART 2: Thread-safe data buffers
// ══════════════════════════════════════════════════════════════════════════

template<typename T>
class LatestBuffer {
    std::optional<T> data_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<int> count_{0};

public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            data_ = std::move(item);
            ++count_;
        }
        cv_.notify_all();
    }

    // Returns latest item; blocks until available or timeout
    std::optional<T> wait_and_get(std::chrono::milliseconds timeout = 50ms) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (cv_.wait_for(lock, timeout, [this]{ return data_.has_value(); })) {
            auto copy = *data_;
            data_.reset();
            return copy;
        }
        return std::nullopt;  // timeout
    }

    int total() const { return count_.load(); }
};

// ══════════════════════════════════════════════════════════════════════════
// PART 3: Simulated Sensor Drivers (jthread with stop_token)
// ══════════════════════════════════════════════════════════════════════════

class SensorDriver {
public:
    std::string name;
    std::atomic<int> published{0};

protected:
    std::mutex print_mtx_;
    std::mutex& global_print_;

public:
    explicit SensorDriver(const std::string& n, std::mutex& pm)
        : name(n), global_print_(pm) {}

    void print(const std::string& msg) {
        std::lock_guard<std::mutex> lk(global_print_);
        std::cout << "[" << name << "] " << msg << "\n";
    }
};

class LidarDriver : public SensorDriver {
    LatestBuffer<LidarData>& buf_;
    JThread thread_;

public:
    LidarDriver(LatestBuffer<LidarData>& buf, std::mutex& pm)
        : SensorDriver("LiDAR", pm), buf_(buf)
    {
        thread_ = JThread([this](StopToken st) { run(st); });
        print("driver started @ 10Hz");
    }

    ~LidarDriver() { print("driver stopping"); }

private:
    void run(StopToken st) {
        std::mt19937 rng(1);
        std::uniform_real_distribution<float> rdist(0.5f, 10.0f);
        TimeNs t = 0;
        while (!st.stop_requested()) {
            buf_.push({t, rdist(rng), 0.0f});
            ++published;
            t += 100'000'000;  // 100ms in ns
            std::this_thread::sleep_for(100ms);
        }
    }
};

class ImuDriver : public SensorDriver {
    LatestBuffer<ImuData>& buf_;
    JThread thread_;

public:
    ImuDriver(LatestBuffer<ImuData>& buf, std::mutex& pm)
        : SensorDriver("IMU", pm), buf_(buf)
    {
        thread_ = JThread([this](StopToken st) { run(st); });
        print("driver started @ 100Hz");
    }

    ~ImuDriver() { print("driver stopping"); }

private:
    void run(StopToken st) {
        std::mt19937 rng(2);
        std::normal_distribution<double> ndist(0.0, 0.01);
        TimeNs t = 0;
        while (!st.stop_requested()) {
            buf_.push({t,
                ndist(rng), ndist(rng), 9.81 + ndist(rng),
                ndist(rng), ndist(rng), ndist(rng)});
            ++published;
            t += 10'000'000;  // 10ms
            std::this_thread::sleep_for(10ms);
        }
    }
};

class CameraDriver : public SensorDriver {
    LatestBuffer<CameraData>& buf_;
    JThread thread_;

public:
    CameraDriver(LatestBuffer<CameraData>& buf, std::mutex& pm)
        : SensorDriver("Camera", pm), buf_(buf)
    {
        thread_ = JThread([this](StopToken st) { run(st); });
        print("driver started @ 30Hz");
    }

    ~CameraDriver() { print("driver stopping"); }

private:
    void run(StopToken st) {
        std::mt19937 rng(3);
        std::uniform_int_distribution<int> obj_dist(0, 5);
        TimeNs t = 0;
        while (!st.stop_requested()) {
            buf_.push({t, 640, 480, obj_dist(rng)});
            ++published;
            t += 33'333'333;  // ~33ms
            std::this_thread::sleep_for(33ms);
        }
    }
};

// ══════════════════════════════════════════════════════════════════════════
// PART 4: Fusion Node
// ══════════════════════════════════════════════════════════════════════════

class FusionNode {
    LatestBuffer<LidarData>&   lidar_buf_;
    LatestBuffer<ImuData>&     imu_buf_;
    LatestBuffer<CameraData>&  cam_buf_;

    std::mutex&                print_mutex_;
    std::atomic<int>           fused_count_{0};
    std::vector<FusedEstimate> history_;
    std::mutex                 history_mutex_;

    JThread               thread_;

public:
    FusionNode(LatestBuffer<LidarData>& lb,
               LatestBuffer<ImuData>&   ib,
               LatestBuffer<CameraData>& cb,
               std::mutex& pm)
        : lidar_buf_(lb), imu_buf_(ib), cam_buf_(cb), print_mutex_(pm)
    {
        thread_ = JThread([this](StopToken st) { run(st); });
        log("fusion node started");
    }

    ~FusionNode() { log("fusion node stopping"); }

    int fused_count() const { return fused_count_.load(); }

    void print_summary() {
        std::lock_guard<std::mutex> lk(history_mutex_);
        log("==== Fusion Summary (" + std::to_string(history_.size()) + " estimates) ====");
        for (size_t i = 0; i < std::min(history_.size(), size_t(5)); ++i) {
            const auto& e = history_[i];
            std::lock_guard<std::mutex> lk2(print_mutex_);
            std::cout << "  [est #" << i << "] x=" << e.x
                      << " y=" << e.y << " vx=" << e.vx
                      << " conf=" << e.confidence
                      << " src=" << e.source << "\n";
        }
        if (history_.size() > 5) {
            std::lock_guard<std::mutex> lk2(print_mutex_);
            std::cout << "  ... and " << history_.size()-5 << " more\n";
        }
    }

private:
    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lk(print_mutex_);
        std::cout << "[Fusion] " << msg << "\n";
    }

    void run(StopToken st) {
        static std::mt19937 rng(42);
        static std::uniform_real_distribution<double> pos(-0.1, 0.1);

        double x = 0.0, y = 0.0, vx = 0.0, vy = 0.0;

        while (!st.stop_requested()) {
            // Collect latest data from each sensor (with timeout)
            auto lidar = lidar_buf_.wait_and_get(20ms);
            auto imu   = imu_buf_.wait_and_get(5ms);
            auto cam   = cam_buf_.wait_and_get(10ms);

            std::string sources;
            int confidence = 0;

            if (lidar) {
                x  += lidar->range * 0.1;  // simplified integration
                confidence += 40;
                sources += "L";
            }
            if (imu) {
                vx += imu->ax * 0.01;       // simplified integration
                vy += imu->ay * 0.01;
                x  += vx * 0.01;
                y  += vy * 0.01;
                confidence += 30;
                sources += "I";
            }
            if (cam) {
                confidence += 30;
                sources += "C(" + std::to_string(cam->detected_objects) + ")";
            }

            if (confidence > 0) {
                FusedEstimate est{0, x + pos(rng), y + pos(rng), vx, vy, confidence, sources};
                {
                    std::lock_guard<std::mutex> lk(history_mutex_);
                    history_.push_back(est);
                }
                ++fused_count_;

                if (fused_count_ % 5 == 0) {
                    log("fused #" + std::to_string(fused_count_.load())
                        + " src=" + sources
                        + " pos=(" + std::to_string(x).substr(0,5)
                        + "," + std::to_string(y).substr(0,5) + ")"
                        + " conf=" + std::to_string(confidence));
                }
            }
        }
    }
};

// ══════════════════════════════════════════════════════════════════════════
// MAIN
// ══════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "===== Multi-Sensor Fusion Simulation =====\n\n";

    LatestBuffer<LidarData>   lidar_buf;
    LatestBuffer<ImuData>     imu_buf;
    LatestBuffer<CameraData>  cam_buf;
    std::mutex                print_mutex;

    // Start sensor drivers (jthread — auto stop on destruction)
    LidarDriver  lidar(lidar_buf, print_mutex);
    ImuDriver    imu(imu_buf, print_mutex);
    CameraDriver camera(cam_buf, print_mutex);

    // Start fusion node
    FusionNode fusion(lidar_buf, imu_buf, cam_buf, print_mutex);

    // Run for 500ms
    std::cout << "\n[Main] Running for 500ms...\n\n";
    std::this_thread::sleep_for(500ms);

    std::cout << "\n[Main] Stopping all nodes...\n";
    // All jthreads stopped and joined in reverse construction order

    // Print summary before destructors run
    // (fusion still valid here)
    std::cout << "\n";
    fusion.print_summary();

    std::cout << "\n[Main] Statistics:\n";
    {
        std::lock_guard<std::mutex> lk(print_mutex);
        std::cout << "  LiDAR published:  " << lidar.published  << "\n";
        std::cout << "  IMU published:    " << imu.published    << "\n";
        std::cout << "  Camera published: " << camera.published << "\n";
        std::cout << "  Fused estimates:  " << fusion.fused_count() << "\n";
    }

    std::cout << "\n[Main] Cleanup (RAII — all jthreads auto-join)...\n";
    return 0;
}
