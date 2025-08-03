// Example for time point and duration in C++ Chrono
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    // Create a time point representing the current time
    auto now = std::chrono::system_clock::now();
    std::cout << "Current time point: " << now.time_since_epoch().count() << " nanoseconds since epoch\n";

    // Create a duration of 2 seconds
    std::chrono::seconds duration(2);
    std::cout << "Duration: " << duration.count() << " seconds\n";

    // Add the duration to the current time point
    auto future_time = now + duration;
    std::cout << "Future time point: " << future_time.time_since_epoch().count() << " nanoseconds since epoch\n";

    // Sleep for 2 seconds to simulate waiting
    std::this_thread::sleep_for(duration);
    std::cout << "Slept for 2 seconds.\n";

    return 0;
}