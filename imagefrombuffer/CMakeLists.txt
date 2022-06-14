
#<-------------------------------------------------->
cmake_minimum_required(VERSION 3.17)
project(ImageFromBuffer)
#设置c++的版本并添加多线程的使用
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")

#设置输出文件的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/release)


#添加依赖库路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
#link_directories("C:/Program Files (x86)/Common Files/Intel/Shared Libraries/intel64")
link_directories("C:/Program Files (x86)/Common Files/Intel/Shared Libraries")
#link_directories("${INTEL_DEV_REDIST}")
set(OPENCL_dep_LIBS OpenCL)



# 查找当前目录的所有源文件
set(REQUIRE_CPP 
    ImageFromBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/basic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/cmdparser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/oclobject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/utils.cpp
)

#指定生成目标
add_executable(test ${REQUIRE_CPP})
target_link_libraries(test ${OPENCL_dep_LIBS})
#添加链接库
if (WIN32)
    target_compile_definitions(test PRIVATE -DWIN32_LEAN_AND_MEAN -D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)
else ()
    target_link_libraries(test stdc++fs)
endif ()

