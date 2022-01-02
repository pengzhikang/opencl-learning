# Learning OpenCl

## 1. complier command step
- mkdir <one sample>
- chmod +x run.sh
- ./run.sh

## 2. Your maybe need to gedit sample' CMakeLists.txt to run your computer
### 2.1. opencl include path
- include_directories("your opencl include path")
- eg: include_directories(/usr/local/cuda/include)
### 2.1. opencl lib path
- link_directories("your opencl lib path")
- eg: link_directories(/usr/local/cuda/lib64)