cmake_minimum_required(VERSION 3.14)
project(LithiumMatcher)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========================
# The Main Engine Binary
# ========================
add_executable(lithium_matcher src/main.cpp src/OrderBook.cpp)

# Apply optimization flags ONLY to our own targets (not to GTest).
# This avoids the MSVC /O2 vs /RTC1 conflict.
if(MSVC)
    target_compile_options(lithium_matcher PRIVATE /O2 /W4)
else()
    target_compile_options(lithium_matcher PRIVATE -O3 -pthread -Wall -Wextra)
endif()


# Google Test Setup

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


# The Test Binary

add_executable(tests tests/test_orderbook.cpp src/OrderBook.cpp)
target_link_libraries(tests gtest_main)

enable_testing()
add_test(NAME LithiumTests COMMAND tests)