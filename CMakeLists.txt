cmake_minimum_required(VERSION 3.16)

project(network-monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/crowcpp/Crow.git
    GIT_TAG v1.2.1
)

FetchContent_MakeAvailable(Crow)

find_package(Threads REQUIRED)

add_library(system_info
    src/system_info.cpp
)

target_include_directories(system_info
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(network-monitor
    src/main.cpp
)

target_link_libraries(network-monitor
    PRIVATE
    Crow::Crow
    Threads::Threads
    system_info
)

include(CTest)

if(BUILD_TESTING)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )

    FetchContent_MakeAvailable(googletest)

    add_executable(system_info_tests
        tests/test_system_info.cpp
    )

    target_link_libraries(system_info_tests
        PRIVATE
        system_info
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(system_info_tests)
endif()
