cmake_minimum_required(VERSION 3.0)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

# 配置两个头文件的搜索路径
include_directories(.)    # 加这句，使得在tsets文件中cpp文件，引用sylar文件中的头文件，就可以使用<syalr/log.h>的形式了，而不用<../sylar/log.h>了
include_directories(/usr/local/include)   

# 链接yaml-cpp动态库
link_directories(/usr/local/lib)

set(LIB_SRC
    sylar/log.cpp
    sylar/util.cpp
    sylar/config.cpp
)

add_library(sylar SHARED ${LIB_SRC})

add_executable(test_log tests/test_log.cpp)
add_dependencies(test_log sylar)
target_link_libraries(test_log sylar yaml-cpp)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
target_link_libraries(test_config sylar yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)