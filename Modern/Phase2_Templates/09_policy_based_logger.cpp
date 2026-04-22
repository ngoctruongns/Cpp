/**
 * PHASE 2 - Bài 09: Policy-Based Logger
 *
 * Mục tiêu:
 *  - Policy-based design: dùng template params làm behavior policies
 *  - Tách biệt Output policy (console/file/null) và Format policy (plain/json/ros)
 *  - Zero-cost abstraction: compiler inlines policy methods
 *  - So sánh với virtual dispatch
 *
 * Compile: g++ -std=c++17 09_policy_based_logger.cpp -o out
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>
#include <mutex>

// ══════════════════════════════════════════════════════════════════════════
// OUTPUT POLICIES — where to write
// ══════════════════════════════════════════════════════════════════════════

// Policy 1: Write to stdout
struct ConsoleOutput {
    void write(const std::string& msg) const {
        std::cout << msg << '\n';
    }
};

// Policy 2: Write to stderr
struct StderrOutput {
    void write(const std::string& msg) const {
        std::cerr << msg << '\n';
    }
};

// Policy 3: Discard all output (null/silent mode — useful in tests)
struct NullOutput {
    void write(const std::string&) const { /* discard */ }
};

// Policy 4: Write to file
class FileOutput {
    mutable std::ofstream file_;
public:
    explicit FileOutput(const std::string& path)
        : file_(path, std::ios::app)
    {
        if (!file_) throw std::runtime_error("Cannot open log file: " + path);
    }
    void write(const std::string& msg) const {
        file_ << msg << '\n';
        file_.flush();
    }
};

// ══════════════════════════════════════════════════════════════════════════
// FORMAT POLICIES — how to format messages
// ══════════════════════════════════════════════════════════════════════════

enum class Level { DEBUG, INFO, WARN, ERROR };

inline const char* level_str(Level l) {
    switch (l) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO ";
        case Level::WARN:  return "WARN ";
        case Level::ERROR: return "ERROR";
    }
    return "?????";
}

// Policy 1: Plain text format
struct PlainFormat {
    std::string format(Level level, const std::string& logger_name,
                       const std::string& msg) const {
        return std::string("[") + level_str(level) + "][" + logger_name + "] " + msg;
    }
};

// Policy 2: JSON format (useful for log aggregators)
struct JsonFormat {
    std::string format(Level level, const std::string& logger_name,
                       const std::string& msg) const {
        // Simple JSON: {"level":"INFO","logger":"xyz","msg":"..."}
        return "{\"level\":\"" + std::string(level_str(level)) +
               "\",\"logger\":\"" + logger_name +
               "\",\"msg\":\"" + msg + "\"}";
    }
};

// Policy 3: ROS2-style format: [level] [timestamp] [logger_name]: msg
struct ROS2Format {
    std::string format(Level level, const std::string& logger_name,
                       const std::string& msg) const {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

        std::ostringstream oss;
        oss << "[" << level_str(level) << "] [" << ms << "] ["
            << logger_name << "]: " << msg;
        return oss.str();
    }
};

// ══════════════════════════════════════════════════════════════════════════
// FILTER POLICIES — which messages to pass through
// ══════════════════════════════════════════════════════════════════════════

struct NoFilter {
    bool should_log(Level) const { return true; }
};

struct InfoAndAbove {
    bool should_log(Level l) const {
        return l >= Level::INFO;
    }
};

struct WarnAndAbove {
    bool should_log(Level l) const {
        return l >= Level::WARN;
    }
};

// ══════════════════════════════════════════════════════════════════════════
// LOGGER — policy-based class template
// ══════════════════════════════════════════════════════════════════════════

template<
    typename OutputPolicy  = ConsoleOutput,
    typename FormatPolicy  = PlainFormat,
    typename FilterPolicy  = NoFilter
>
class Logger : private OutputPolicy,
               private FormatPolicy,
               private FilterPolicy
{
    std::string name_;

public:
    // Constructor-forwarding for stateful policies (e.g., FileOutput)
    template<typename... OutputArgs>
    explicit Logger(const std::string& name, OutputArgs&&... out_args)
        : OutputPolicy(std::forward<OutputArgs>(out_args)...)
        , name_(name)
    {}

    void log(Level level, const std::string& msg) {
        if (!FilterPolicy::should_log(level)) return;
        std::string formatted = FormatPolicy::format(level, name_, msg);
        OutputPolicy::write(formatted);
    }

    void debug(const std::string& msg) { log(Level::DEBUG, msg); }
    void info (const std::string& msg) { log(Level::INFO,  msg); }
    void warn (const std::string& msg) { log(Level::WARN,  msg); }
    void error(const std::string& msg) { log(Level::ERROR, msg); }
};

// ══════════════════════════════════════════════════════════════════════════
// DEMOS
// ══════════════════════════════════════════════════════════════════════════

void demo_plain_console() {
    std::cout << "\n=== Plain Console Logger ===\n";
    Logger<> logger("main_node");  // all defaults
    logger.debug("robot starting up");
    logger.info("sensor initialized");
    logger.warn("battery below 20%");
    logger.error("lidar connection lost");
}

void demo_json_console() {
    std::cout << "\n=== JSON Console Logger ===\n";
    Logger<ConsoleOutput, JsonFormat> logger("lidar_node");
    logger.info("scan received");
    logger.warn("out of range value detected");
}

void demo_ros2_format() {
    std::cout << "\n=== ROS2-style Logger ===\n";
    Logger<ConsoleOutput, ROS2Format> logger("slam_node");
    logger.info("map initialized");
    logger.warn("loop closure rejected");
    logger.error("localization diverged");
}

void demo_filtered_logger() {
    std::cout << "\n=== Filtered Logger (WARN and above only) ===\n";
    Logger<ConsoleOutput, PlainFormat, WarnAndAbove> logger("filtered_node");
    logger.debug("this is hidden");   // filtered out
    logger.info("this is hidden");    // filtered out
    logger.warn("this appears");
    logger.error("this appears too");
}

void demo_null_logger() {
    std::cout << "\n=== Null Logger (silent, for testing) ===\n";
    Logger<NullOutput> logger("silent_node");
    logger.info("this goes nowhere");
    logger.error("no output at all");
    std::cout << "(no output above — NullOutput discards everything)\n";
}

void demo_file_logger() {
    std::cout << "\n=== File Logger ===\n";
    try {
        Logger<FileOutput, ROS2Format> logger("nav_node", "/tmp/ros_log.txt");
        logger.info("navigating to goal");
        logger.warn("obstacle detected");
        logger.error("path planning failed");
        std::cout << "Written to /tmp/ros_log.txt\n";
    } catch (const std::exception& e) {
        std::cerr << "File logger error: " << e.what() << "\n";
    }
}

// ── Type aliases for convenience ──────────────────────────────────────────
using ROS2Logger   = Logger<ConsoleOutput, ROS2Format,   InfoAndAbove>;
using DebugLogger  = Logger<ConsoleOutput, PlainFormat,  NoFilter>;
using SilentLogger = Logger<NullOutput>;

void demo_type_aliases() {
    std::cout << "\n=== Type Aliases ===\n";
    ROS2Logger ros_log("my_node");
    ros_log.debug("hidden — INFO filter");  // filtered
    ros_log.info("visible info message");
    ros_log.error("error visible");
}

int main() {
    demo_plain_console();
    demo_json_console();
    demo_ros2_format();
    demo_filtered_logger();
    demo_null_logger();
    demo_file_logger();
    demo_type_aliases();
    return 0;
}
