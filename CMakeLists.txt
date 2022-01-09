cmake_minimum_required(VERSION 3.10.0)

project(RegeditPP VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
find_package(GTest CONFIG REQUIRED)
add_executable(main test.cpp) #This is the testing executable
target_link_libraries(main PRIVATE GTest::gtest GTest::gtest_main) #Link it to the google test library