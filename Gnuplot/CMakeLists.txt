cmake_minimum_required(VERSION  2.8)
project(gnu-plot CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Boost 1.40.0 COMPONENTS filesystem system iostreams REQUIRED)
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/externals/gnuplot-iostream")

file(GLOB SRCS src/*.cpp)
add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})