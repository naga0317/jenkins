cmake_minimum_required(VERSION 3.16)

project(network-monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(network-monitor
    src/main.cpp
)

find_package(Threads REQUIRED)

target_include_directories(network-monitor
    PRIVATE
    /home/naga/Crow/include
)

target_link_libraries(network-monitor
    PRIVATE
    Threads::Threads
)
