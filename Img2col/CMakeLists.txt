
#<-------------------------------------------------->
cmake_minimum_required(VERSION 3.19)
#设置c++的版本并添加多线程的使用
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

#设置输出文件的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../)


#添加opencv、acl、tengine的依赖库路径
link_directories(/usr/lib)
set(OPENCL_dep_LIBS OpenCL)
#添加opencv、acl、tengine的头文件路径



# 查找当前目录的所有源文件
set(REQUIRE_CPP 
    main.cpp    
)

#指定生成目标
add_executable(test ${REQUIRE_CPP})
target_link_libraries(test ${OPENCL_dep_LIBS})
#添加链接库,包括opencv和qt5
target_link_libraries(test stdc++fs)

