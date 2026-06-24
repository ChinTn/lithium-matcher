cmake_minimum_required(VERSION 3.14)
project(LithiumMatcher)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags (-O3) are critical for HFT projects. 
# We also include pthread for the ring buffer threading.
if(MSVC)
    add_compile_options(/O2 /W4)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread -Wall -Wextra")
endif()

add_executable(lithium_matcher src/main.cpp src/OrderBook.cpp)
