cmake_minimum_required(VERSION 3.13.4)
project(Task VERSION 1.0)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CPP_COMPILER g++)

add_executable(Task main.cpp bmpreader.cpp shared_container.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Task ${CMAKE_THREAD_LIBS_INIT})

enable_testing()

add_test(Tigers Task -b tiger.bmp -y Tiger.yuv -w 1920 -h 1080 -o Tigers.yuv)
add_test(Chevalier Task -b chevalier.bmp -y Tiger.yuv -w 1920 -h 1080 -o Chevalier.yuv)
