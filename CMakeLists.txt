cmake_minimum_required(VERSION 3.13.0)
project(sqlite_example CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(3rdparty/sqlite3)

find_package(nlohmann_json 3.5.0 EXACT REQUIRED)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE sqlite3::sqlite3 nlohmann_json)