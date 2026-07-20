cmake_minimum_required(VERSION 3.10)
project(Singleton)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Example 1: Basic Singleton
add_executable(01_basic_singleton 01_basic_singleton.cpp)

# Example 2: Thread-safe Singleton with Double-Checked Locking
add_executable(02_thread_safe_singleton 02_thread_safe_singleton.cpp)
target_link_libraries(02_thread_safe_singleton pthread)

# Example 3: Meyers Singleton (Recommended)
add_executable(03_meyers_singleton 03_meyers_singleton.cpp)
target_link_libraries(03_meyers_singleton pthread)

# Example 4: Singleton with shared_ptr
add_executable(04_shared_singleton 04_shared_singleton.cpp)
target_link_libraries(04_shared_singleton pthread)

# Example 5: Practical Example
add_executable(05_practical_example 05_practical_example.cpp)

# Example 6: Comparison and Anti-patterns
add_executable(06_comparison_and_antipatterns 06_comparison_and_antipatterns.cpp)

# Example 7: Template-based Singleton
add_executable(07_template_singleton 07_template_singleton.cpp)

# Custom target to run all examples
add_custom_target(run_all
    COMMAND echo "\\n=== Running 01_basic_singleton ===" && ./01_basic_singleton
    COMMAND echo "\\n=== Running 02_thread_safe_singleton ===" && ./02_thread_safe_singleton
    COMMAND echo "\\n=== Running 03_meyers_singleton ===" && ./03_meyers_singleton
    COMMAND echo "\\n=== Running 04_shared_singleton ===" && ./04_shared_singleton
    COMMAND echo "\\n=== Running 05_practical_example ===" && ./05_practical_example
    COMMAND echo "\\n=== Running 06_comparison_and_antipatterns ===" && ./06_comparison_and_antipatterns
    COMMAND echo "\\n=== Running 07_template_singleton ===" && ./07_template_singleton
    DEPENDS 01_basic_singleton 02_thread_safe_singleton 03_meyers_singleton
            04_shared_singleton 05_practical_example 06_comparison_and_antipatterns
            07_template_singleton
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
