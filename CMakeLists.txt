cmake_minimum_required(VERSION 3.13.4)

project(Task)

set(CMAKE_CXX_STANDARD 11)
set(CONFIGURE_FILE config.h)

#set(MODE HDTV)
set(MODE SDTV)

set(Main main.cpp)
set(BMPReader bmpreader.cpp)
set(SharedContainer shared_container.cpp)

add_executable(${PROJECT_NAME} ${Main} ${BMPReader} ${SharedContainer})

configure_file("${CONFIGURE_FILE}.in" "${PROJECT_SOURCE_DIR}/include/${CONFIGURE_FILE}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Task ${CMAKE_THREAD_LIBS_INIT})


enable_testing()

add_test(NAME Test1 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${PROJECT_NAME} -b tiger.bmp -y Tiger.yuv -w 1920 -h 1080 -o Tigers.yuv)
add_test(NAME Test2 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${PROJECT_NAME} -b chevalier.bmp -y Tiger.yuv -w 1920 -h 1080 -o Chevalier.yuv)
