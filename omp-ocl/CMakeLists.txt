
#<-------------------------------------------------->
cmake_minimum_required(VERSION 3.19)


#设置输出文件的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../)

include_directories(${PROJECT_BINARY_DIR}/../opencl/include)
#增加omp的使用
find_package(OpenMP REQUIRED)
if(OPENMP_FOUND)
    message("Found OpenMP")
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS})
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS})
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
#添加opencv、acl、tengine的依赖库路径
link_directories(/usr/lib)
link_directories(${PROJECT_BINARY_DIR}/../opencl//lib)
# set(OPENCV_dep_LIBS opencv_imgproc opencv_imgcodecs opencv_core opencv_dnn opencv_highgui opencv_videoio)
set(OPENCL_dep_LIBS OpenCL)
#添加opencv、acl、tengine的头文件路径



# 查找当前目录的所有源文件
set(REQUIRE_CPP 
    main.cpp    
)

#指定生成目标
add_executable(test ${REQUIRE_CPP})
target_link_libraries(test ${OPENCL_dep_LIBS})
# target_link_libraries(test ${OPENCV_dep_LIBS})
#添加链接库,包括opencv和qt5
target_link_libraries(test stdc++fs)

