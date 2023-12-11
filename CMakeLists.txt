cmake_minimum_required(VERSION 3.16)

project(ballz VERSION 3000)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(engine)

add_executable(game main.cpp)
target_link_libraries(game PUBLIC ENGINE)
