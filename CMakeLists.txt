cmake_minimum_required(VERSION 3.0)
project(sylar)

include (cmake/utils.cmake)

add_definitions(-Wno-builtin-macro-redefined)

set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall \
    -Wno-deprecated -Werror -Wno-unused-function")  # 注意不能回车换行，要类似于宏里面的换行，加\换行

# 配置两个头文件的搜索路径
include_directories(.)    # 加这句，使得在tsets文件中cpp文件，引用sylar文件中的头文件，就可以使用<syalr/log.h>的形式了，而不用<../sylar/log.h>了
include_directories(/usr/local/include)   

# 链接yaml-cpp动态库
link_directories(/usr/local/lib)

set(LIB_SRC
    sylar/config.cpp
    sylar/fiber.cpp
    sylar/log.cpp
    sylar/scheduler.cpp
    sylar/thread.cpp
    sylar/util.cpp
)

add_library(sylar SHARED ${LIB_SRC})
redefine_file_macro(sylar)  # __FILE__

set(LIB_LIB
    sylar
    yaml-cpp
    pthread
)

add_executable(test_log tests/test_log.cpp)
add_dependencies(test_log sylar)
redefine_file_macro(test_log)
target_link_libraries(test_log ${LIB_LIB})
# target_link_libraries(test_log sylar yaml-cpp pthread)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
redefine_file_macro(test_config)
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread sylar)
redefine_file_macro(test_thread)
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util sylar)
redefine_file_macro(test_util)
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber sylar)
redefine_file_macro(test_fiber)
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler sylar)
redefine_file_macro(test_scheduler)
target_link_libraries(test_scheduler ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)