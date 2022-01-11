#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<iostream>
#include<string>

#include<CL/cl.h>
#include<omp.h>
template <typename T>
void get_device_info(cl_device_id device, cl_device_info param_name, std::string name)
{
    T* info;
    size_t info_size = 0;
    clGetDeviceInfo(device, param_name, 0, NULL, &info_size);
    info = (T*)malloc(info_size / sizeof(T));
    std::cout << name <<" info num " << info_size / sizeof(T) << ":";
    clGetDeviceInfo(device, param_name, info_size, info, &info_size);
    
    for(size_t i = 0; i < info_size / sizeof(T); i++)
    {
        std::cout << info[i] <<",";
    }
    free(info);
    std::cout << std::endl;
}

void print_device(cl_platform_id platform)
{
    cl_int err;
    cl_device_id* devices;
    cl_uint num_devices;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
    devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);
    for(size_t i = 0; i < num_devices; i++)
    {
        get_device_info<size_t>(devices[i], CL_DEVICE_MAX_WORK_ITEM_SIZES, "cl_device_max_work_item_sizes");
        get_device_info<cl_device_type>(devices[i], CL_DEVICE_TYPE, "cl_device_type");
        get_device_info<cl_uint>(devices[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, "cl_device_max_clock_frequency");
        get_device_info<cl_ulong>(devices[i], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "cl_device_global_mem_cacheline_size");
        get_device_info<cl_device_fp_config>(devices[i], CL_DEVICE_DOUBLE_FP_CONFIG, "CL_DEVICE_DOUBLE_FP_CONFIG");
        
    }
    free(devices);
}

// 进行omp ocl的组合问题实践
/*
进行操作之后
*/
void omp_ocl(cl_platform_id platform)
{
    cl_int err;
    cl_device_id* devices;
    cl_uint num_devices;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
    devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);
    // for(size_t i = 0; i < num_devices; i++)
    // {
    //     get_device_info<size_t>(devices[i], CL_DEVICE_MAX_WORK_ITEM_SIZES, "cl_device_max_work_item_sizes");
    //     get_device_info<cl_device_type>(devices[i], CL_DEVICE_TYPE, "cl_device_type");
    //     get_device_info<cl_uint>(devices[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, "cl_device_max_clock_frequency");
    //     get_device_info<cl_ulong>(devices[i], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "cl_device_global_mem_cacheline_size");
    //     get_device_info<cl_device_fp_config>(devices[i], CL_DEVICE_DOUBLE_FP_CONFIG, "CL_DEVICE_DOUBLE_FP_CONFIG");
    // }
    cl_int errcode;
    cl_context Context = clCreateContext(NULL, num_devices, devices, NULL, NULL, &errcode);
    if(errcode != CL_SUCCESS || Context == NULL)
    {
        printf("create context failed(errcode=%d)\n", errcode);
    }
    omp_set_num_threads(num_devices);
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int res[10];
        for(size_t j = 0; j < 10; j++)
            res[j] = tid;
        cl_command_queue CommandQueue = clCreateCommandQueue(Context, devices[tid], CL_QUEUE_PROFILING_ENABLE, &errcode);
        if (errcode != CL_SUCCESS)
        {
            std::cout << "Error: Can not create CommandQueue" << std::endl;
        }
        //-------------------------7. 创建输入输出内核内存对象--------------------------------
        // 创建内存对象
        cl_mem r = clCreateBuffer(
            Context,
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, // 输入内存为只读，并可以从宿主机内存复制到设备内存
            10 * sizeof(int),					  // 输入内存空间大小
            res,
            &errcode);
        if (errcode != CL_SUCCESS)
        {
            std::cout << errcode << std::endl;
            std::cout << "Error creating memory objects" << std::endl;
        }
        printf("itd-----%d\n", tid);
        clReleaseMemObject(r);
        clReleaseCommandQueue(CommandQueue);
    }
    clReleaseContext(Context);
    for(size_t i = 0; i < num_devices; i++)
        clReleaseDevice(devices[i]);
    free(devices);
}

int main()
{
    cl_platform_id *platform;
    cl_uint num_platform;
    cl_int err;
    err = clGetPlatformIDs(0,NULL, &num_platform);
    printf("err:%d\n", err);
    platform = (cl_platform_id *)malloc(sizeof (cl_platform_id) *
                                         num_platform);
    printf("CL Platform Num:%d\n", num_platform);
    err = clGetPlatformIDs(num_platform,platform, NULL);
    for(int i = 0; i < num_platform; i++)
    {
        size_t size;
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_NAME,0,NULL,&size);
        char *PName = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_NAME,size,
                                PName,NULL);
        printf("\nCL_PLATFORM_NAME：%s\n",PName);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_VENDOR,
                                0,NULL,&size);
        char *PVendor = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_VENDOR,
                                size,PVendor, NULL);
        printf("CL_PLATFORM_VENDOR：%s\n", PVendor);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_VERSION,
                                0,NULL,&size);
        char *PVersion = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_VERSION,
                                size,PVersion, NULL);
        printf("CL_PLATFORM_VERSION：%s\n", PVersion);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_PROFILE,0,
                                NULL,&size);
        char *PProfile = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_PROFILE,
                                size,PProfile, NULL);
        printf("CL_PLATFORM_PROFILE：%s\n", PProfile);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_EXTENSIONS,
                                0,NULL,&size);
        char *PExten = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_EXTENSIONS,
                                size,PExten,NULL);
        printf("CL_PLATFORM_EXTENSIONS：%s\n", PExten);
        // print_device(platform[i]);
        omp_ocl(platform[i]);
        free(PName);
        free(PVendor);
        free(PVersion);
        free(PProfile);
        free(PExten);
    }
}