cmake_minimum_required(VERSION 3.5)

# Tên dự án
project(my_project)

# Thêm thư mục chứa file header
include_directories(include)

# Thêm file thực thi và tên file thực thi
add_executable(my_executable src/main.cpp)

# Liên kết file thực thi với thư viện my_class
target_link_libraries(my_executable my_class)
