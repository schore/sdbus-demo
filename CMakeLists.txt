cmake_minimum_required (VERSION 3.11)
project(api-server)


set(CMAKE_CXX_STANDARD 17)

find_package(sdbus-c++ CONFIG REQUIRED)

add_executable(server main.cpp)
target_link_libraries(server
  PUBLIC SDBusCpp::sdbus-c++)
