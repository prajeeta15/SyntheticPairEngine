cmake_minimum_required(VERSION 3.16)
project(SyntheticPairEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} Threads::Threads)

# Tests
enable_testing()
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
if(TEST_SOURCES)
    add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES} ${SOURCES})
    target_link_libraries(${PROJECT_NAME}_tests Threads::Threads)
    add_test(NAME unit_tests COMMAND ${PROJECT_NAME}_tests)
endif()
