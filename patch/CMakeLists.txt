cmake_minimum_required(VERSION 3.0)
project(patch)

set(COM_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wextra -Wall -g -ggdb -Wunused-function -std=c++11 -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wextra -O0 -Wall -g -ggdb -std=c++11 -fPIC")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")

set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wextra -Wall -g -ggdb -Wunused-function -fPIC")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wextra -O0 -Wall -g -ggdb -std=c++11 -fPIC")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG}")

#SET(EXECUTABLE_OUTPUT_PATH ${COM_ROOT_PATH}/)       #设置可执行文件的输出目录
#SET(LIBRARY_OUTPUT_PATH ${COM_ROOT_PATH}/)           #设置库文件的输出目录

set(SOURCE_FILES "")
aux_source_directory(${COM_ROOT_PATH} SOURCE_FILES)

#设定头文件路径
include_directories(${COM_ROOT_PATH})
 
 
#设定链接库的路径（一般使用第三方非系统目录下的库）
set(LINK_LIB_PATH ${LIB_PATH})
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH} ${LINK_LIB_PATH})
 
 
#添加依赖项子目录
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})
 
#生成可执行文件
add_library(patch SHARED ${SOURCE_FILES})

