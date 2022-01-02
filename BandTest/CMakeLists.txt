
#<-------------------------------------------------->
cmake_minimum_required(VERSION 3.19)
#设置c++的版本并添加多线程的使用
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -DUNIX")

#设置输出文件的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(/usr/include)
include_directories(/usr/local/cuda/include)
include_directories(/usr/local/cuda-11.4/samples/common/inc/)


#添加opencl的依赖库路径
link_directories(/usr/lib)
link_directories(/usr/local/cuda/lib64)
set(OPENCL_dep_LIBS OpenCL)
message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
# file(GLOB USER_LIBS_PATH ${PROJECT_SOURCE_DIR}/src/*.cpp)
aux_source_directory(${PROJECT_SOURCE_DIR}/src REQUIRE_CPP)
# 查找当前目录的所有源文件
# set(REQUIRE_CPP 
#     DIR_SRCS
# )

#指定生成目标
add_executable(test ${REQUIRE_CPP})
target_link_libraries(test ${OPENCL_dep_LIBS})
#添加链接库,包括
target_link_libraries(test stdc++fs)

