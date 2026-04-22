/**
 * PHASE 4 - Bài 05: Plugin System (ROS2 pluginlib-like)
 *
 * Mục tiêu:
 *  - Plugin interface + registry pattern
 *  - Factory với string → unique_ptr<Plugin>
 *  - Plugin discovery, versioning
 *  - Giống ROS2 pluginlib: load algorithm plugins at runtime
 *
 * Compile: g++ -std=c++17 05_plugin_system.cpp -o out
 */

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────────────
// PART 1: Plugin interfaces
// ──────────────────────────────────────────────────────────────────────────

struct ScanData {
    std::vector<float> ranges;
    float angle_min, angle_max;
};

struct Pose {
    double x, y, theta;
};

// Global planner plugin interface
class GlobalPlanner {
public:
    virtual ~GlobalPlanner() = default;
    virtual std::string plugin_name() const = 0;
    virtual std::string version()     const { return "1.0.0"; }
    virtual void initialize(const std::unordered_map<std::string, std::string>& params) {}
    virtual std::vector<Pose> plan(const Pose& start, const Pose& goal) = 0;
};

// Costmap filter plugin interface
class CostmapFilter {
public:
    virtual ~CostmapFilter() = default;
    virtual std::string plugin_name() const = 0;
    virtual void process(ScanData& scan) = 0;
};

// ──────────────────────────────────────────────────────────────────────────
// PART 2: Concrete plugin implementations
// ──────────────────────────────────────────────────────────────────────────

class DijkstraPlanner : public GlobalPlanner {
    double step_size_ = 0.1;
public:
    std::string plugin_name() const override { return "dijkstra_planner"; }
    std::string version()     const override { return "2.0.1"; }

    void initialize(const std::unordered_map<std::string, std::string>& params) override {
        auto it = params.find("step_size");
        if (it != params.end()) step_size_ = std::stod(it->second);
        std::cout << "[DijkstraPlanner] init step_size=" << step_size_ << "\n";
    }

    std::vector<Pose> plan(const Pose& start, const Pose& goal) override {
        std::cout << "[DijkstraPlanner] planning from ("
                  << start.x << "," << start.y << ") to ("
                  << goal.x  << "," << goal.y  << ")\n";
        // Simplified: straight line with step_size
        std::vector<Pose> path;
        double dx = (goal.x - start.x);
        double dy = (goal.y - start.y);
        double dist = std::sqrt(dx*dx + dy*dy);
        int steps = static_cast<int>(dist / step_size_) + 1;
        for (int i = 0; i <= steps; ++i) {
            double t = static_cast<double>(i) / steps;
            path.push_back({start.x + t*dx, start.y + t*dy, 0.0});
        }
        return path;
    }
};

class AStarPlanner : public GlobalPlanner {
    double heuristic_weight_ = 1.0;
public:
    std::string plugin_name() const override { return "astar_planner"; }

    void initialize(const std::unordered_map<std::string, std::string>& params) override {
        auto it = params.find("heuristic_weight");
        if (it != params.end()) heuristic_weight_ = std::stod(it->second);
        std::cout << "[AStarPlanner] init heuristic_weight=" << heuristic_weight_ << "\n";
    }

    std::vector<Pose> plan(const Pose& start, const Pose& goal) override {
        std::cout << "[AStarPlanner] w=" << heuristic_weight_
                  << " planning (" << start.x << "," << start.y << ") → ("
                  << goal.x << "," << goal.y << ")\n";
        return {{start.x, start.y, 0.0},
                {(start.x+goal.x)/2, (start.y+goal.y)/2, 0.0},
                {goal.x, goal.y, 0.0}};
    }
};

class OutlierFilter : public CostmapFilter {
    float max_range_ = 10.0f;
public:
    std::string plugin_name() const override { return "outlier_filter"; }

    void process(ScanData& scan) override {
        int removed = 0;
        for (auto& r : scan.ranges) {
            if (r > max_range_) { r = max_range_; ++removed; }
        }
        std::cout << "[OutlierFilter] clamped " << removed << " outliers\n";
    }
};

class MedianFilter : public CostmapFilter {
public:
    std::string plugin_name() const override { return "median_filter"; }

    void process(ScanData& scan) override {
        if (scan.ranges.size() < 3) return;
        std::vector<float> smoothed(scan.ranges.size());
        for (size_t i = 1; i < scan.ranges.size() - 1; ++i) {
            std::vector<float> w = {scan.ranges[i-1], scan.ranges[i], scan.ranges[i+1]};
            std::sort(w.begin(), w.end());
            smoothed[i] = w[1];
        }
        smoothed.front() = scan.ranges.front();
        smoothed.back()  = scan.ranges.back();
        scan.ranges = std::move(smoothed);
        std::cout << "[MedianFilter] applied 3-point median\n";
    }
};

// ──────────────────────────────────────────────────────────────────────────
// PART 3: Plugin Registry (ClassLoader simulation)
// ──────────────────────────────────────────────────────────────────────────
template<typename PluginBase>
class PluginRegistry {
    using Factory = std::function<std::unique_ptr<PluginBase>()>;
    std::unordered_map<std::string, Factory> factories_;

public:
    // Register a plugin type
    template<typename PluginImpl>
    void register_plugin(const std::string& name) {
        factories_[name] = []() -> std::unique_ptr<PluginBase> {
            return std::make_unique<PluginImpl>();
        };
        std::cout << "[Registry] registered: " << name << "\n";
    }

    // Create plugin by name
    std::unique_ptr<PluginBase> create(const std::string& name) const {
        auto it = factories_.find(name);
        if (it == factories_.end()) {
            throw std::runtime_error("Plugin not found: " + name);
        }
        return it->second();
    }

    // List all registered plugins
    std::vector<std::string> available() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : factories_) names.push_back(name);
        return names;
    }

    bool has(const std::string& name) const {
        return factories_.count(name) > 0;
    }
};

// ──────────────────────────────────────────────────────────────────────────
// PART 4: Navigation Manager using plugins
// ──────────────────────────────────────────────────────────────────────────
class NavigationManager {
    PluginRegistry<GlobalPlanner>  planner_registry_;
    PluginRegistry<CostmapFilter>  filter_registry_;

    std::unique_ptr<GlobalPlanner>          active_planner_;
    std::vector<std::unique_ptr<CostmapFilter>> active_filters_;

public:
    NavigationManager() {
        // Register all available plugins
        planner_registry_.register_plugin<DijkstraPlanner>("dijkstra_planner");
        planner_registry_.register_plugin<AStarPlanner>("astar_planner");
        filter_registry_.register_plugin<OutlierFilter>("outlier_filter");
        filter_registry_.register_plugin<MedianFilter>("median_filter");
    }

    void set_planner(const std::string& name,
                     const std::unordered_map<std::string, std::string>& params = {}) {
        active_planner_ = planner_registry_.create(name);
        active_planner_->initialize(params);
        std::cout << "[NavManager] planner=" << active_planner_->plugin_name()
                  << " v" << active_planner_->version() << "\n";
    }

    void add_filter(const std::string& name) {
        active_filters_.push_back(filter_registry_.create(name));
        std::cout << "[NavManager] added filter: " << name << "\n";
    }

    std::vector<Pose> navigate(const Pose& start, const Pose& goal, ScanData& scan) {
        if (!active_planner_) throw std::runtime_error("No planner set");

        // Apply filters
        for (auto& f : active_filters_) f->process(scan);

        // Plan
        return active_planner_->plan(start, goal);
    }

    void list_plugins() const {
        std::cout << "[NavManager] Planners: ";
        for (const auto& n : planner_registry_.available()) std::cout << n << " ";
        std::cout << "\n[NavManager] Filters:  ";
        for (const auto& n : filter_registry_.available())  std::cout << n << " ";
        std::cout << "\n";
    }
};

int main() {
    std::cout << "===== Plugin System Demo =====\n\n";

    NavigationManager nav;

    std::cout << "--- Available plugins ---\n";
    nav.list_plugins();

    std::cout << "\n--- Use Dijkstra planner with filters ---\n";
    nav.set_planner("dijkstra_planner", {{"step_size", "0.5"}});
    nav.add_filter("outlier_filter");
    nav.add_filter("median_filter");

    ScanData scan{{0.5f, 20.0f, 1.2f, 15.0f, 0.8f, 1.5f}, -1.57f, 1.57f};
    auto path = nav.navigate({0,0,0}, {2,3,0}, scan);
    std::cout << "Path length: " << path.size() << " waypoints\n";
    for (const auto& p : path)
        std::cout << "  (" << p.x << ", " << p.y << ")\n";

    std::cout << "\n--- Switch to A* planner at runtime ---\n";
    nav.set_planner("astar_planner", {{"heuristic_weight", "1.5"}});

    ScanData scan2{{1.0f, 2.0f, 3.0f}, -1.57f, 1.57f};
    auto path2 = nav.navigate({0,0,0}, {5,5,0}, scan2);
    std::cout << "Path length: " << path2.size() << " waypoints\n";

    std::cout << "\n--- Unknown plugin ---\n";
    try {
        nav.set_planner("unknown_planner");
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }

    return 0;
}
