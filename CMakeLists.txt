cmake_minimum_required(VERSION 3.0)
project(photpatch)

set(COM_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMD_PATH "${COM_ROOT_PATH}/cmd")
set(INC_PATH "${COM_ROOT_PATH}/inc")
set(SRC_PATH "${COM_ROOT_PATH}/src")
set(TEST_PATH "${COM_ROOT_PATH}/test")
set(UTIL_PATH "${COM_ROOT_PATH}/util")
set(LIB_PATH "${COM_ROOT_PATH}/lib")
set(INJECTOR_PATH "${COM_ROOT_PATH}/third-party/injector/include")
set(LIBUWIND_PATH "${COM_ROOT_PATH}/third-party/libunwind/include")
set(YAML_PATH "${COM_ROOT_PATH}/third-party/yaml-cpp/include")
set(LIBUWIND_LIB_PATH "${COM_ROOT_PATH}/third-party/libunwind/src/.libs")


set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wextra -Wall -g -ggdb -Wunused-function -std=c++11 -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wextra -O0 -Wall -g -ggdb -std=c++11 -fPIC")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")

SET(EXECUTABLE_OUTPUT_PATH ${COM_ROOT_PATH}/build/bin)       #设置可执行文件的输出目录
SET(LIBRARY_OUTPUT_PATH ${COM_ROOT_PATH}/build/lib)           #设置库文件的输出目录

set(SOURCE_FILES "")
aux_source_directory(${SRC_PATH} SOURCE_FILES)
aux_source_directory(${CMD_PATH} SOURCE_FILES)
aux_source_directory(${UTIL_PATH} SOURCE_FILES)

message(${YAML_PATH})


#设定头文件路径
include_directories(${INC_PATH}
                    ${UTIL_PATH}
                    ${SRC_PATH}
                    ${INJECTOR_PATH}
                    ${LIBUWIND_PATH}
                    ${YAML_PATH})
 
 
#设定链接库的路径（一般使用第三方非系统目录下的库）
set(LINK_LIB_PATH ${LIB_PATH})
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH} ${LINK_LIB_PATH} ${LIBUWIND_LIB_PATH})
 
 
#添加依赖项子目录
 
#生成可执行文件
add_executable(hotpatch ${SOURCE_FILES})
target_link_libraries(hotpatch  libunwind-ptrace.a libunwind-x86_64.a libunwind.a libyaml-cpp.a  libinjector.a -lpthread -llzma)

if(EXISTS ${CMAKE_INSTALL_PREFIX})
message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
INSTALL(TARGETS hotpatch DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
else()
message("it could set : cmake CMAKE_INSTALL_PREFIX=/usr/local ../")
INSTALL(TARGETS hotpatch DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()

#子项目
add_subdirectory("${COM_ROOT_PATH}/patch")
add_subdirectory("${COM_ROOT_PATH}/target")
