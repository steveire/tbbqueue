cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(TBBQueue)

add_executable(tbbqueue main.cpp)
target_link_libraries(tbbqueue
    /opt/intel/oneapi/tbb/2021.5.1/lib/intel64/gcc4.8/libtbb.so.12.5
    -pthread
)
target_include_directories(tbbqueue PRIVATE
    /opt/intel/oneapi/tbb/2021.5.1/include/
)