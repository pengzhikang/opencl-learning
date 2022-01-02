#include<CL/cl.h>
#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<iostream>
#include<string>
#include<vector>
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

// 打印设备信息
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
    }
    free(devices);
}
// 打印上下文的设备信息
void print_context_info(cl_context context)
{
    cl_uint re_count;
    cl_int err;
    err = clGetContextInfo(context,CL_CONTEXT_REFERENCE_COUNT , sizeof(cl_uint), &re_count, NULL);
    printf("reference_count : %d\n", re_count);
    clRetainContext(context);
    err = clGetContextInfo(context,CL_CONTEXT_REFERENCE_COUNT , sizeof(cl_uint), &re_count, NULL);
    printf("after retain reference_count : %d\n", re_count);
    clReleaseContext(context);
    err = clGetContextInfo(context,CL_CONTEXT_REFERENCE_COUNT , sizeof(cl_uint), &re_count, NULL);
    printf("after release reference_count : %d\n", re_count);
    clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &re_count, NULL);
    printf("devices num : %d\n", re_count);
    cl_device_id* a = (cl_device_id*)malloc(sizeof(cl_device_id) * re_count);
    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(cl_device_id) * re_count, a, NULL);
    printf("get all devices is %s\n", err == CL_SUCCESS?"success":"failed");
    free(a);
}

// 创建命令队列
cl_command_queue create_queue(cl_context context, cl_device_id device)
{
    cl_int err;
    cl_command_queue_properties qp = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE;
    cl_command_queue q = clCreateCommandQueue(context, device, qp, &err);
    if(err == CL_SUCCESS)
    {
        printf("Create Queue success from context with device\n");
    }
    // clReleaseCommandQueue(q);
    return q;
}

// 创建程序对象
cl_program create_program(cl_context context, std::vector<std::string> cl_file)
{
    cl_uint file_num = cl_file.size();
    // char *file_string[1];
    char** file_string = (char**)malloc(sizeof(char*)*file_num);
    size_t* file_len = (size_t*)malloc(sizeof(size_t)*file_num);
    FILE* file_handle;
    for(size_t i = 0; i < file_num; i++)
    {
        file_handle = fopen(cl_file[i].c_str(), "r");
        fseek(file_handle, 0, SEEK_END);
        file_len[i] = ftell(file_handle);
        rewind(file_handle);
        file_string[i] = (char*)malloc(file_len[i] + 1);
        fread(file_string[i], sizeof(char), file_len[i], file_handle);
        file_string[i][file_len[i]] = '\0';
        fclose(file_handle);
    }
    // 创建程序对象
    cl_int err;
    // 需要把file_string本来的char**类型转换成const char**，才能够进行成功建立程序对象
    cl_program p1 = clCreateProgramWithSource(context, file_num, (const char**)file_string, file_len, &err);
    if(err == CL_SUCCESS)
    {
        printf("create program object success\n");
    }
    return p1;
}

// 查询程序对象
template <typename T>
cl_int print_program_info(cl_program p1, cl_program_info name, std::string n, bool just_one = false)
{
    cl_int err;
    size_t num;
    err = clGetProgramInfo(p1, name, 0, NULL, &num);
    T* p_ptr = (T*) malloc(num);
    err = clGetProgramInfo(p1, name, num, p_ptr, &num);
    std::cout << n << ":";
    // if (!just_one)
    // {
        for(size_t i = 0; i < num / sizeof(T); i++)
        {
            std::cout << p_ptr[i];
            if (!just_one)
            {
                std::cout << ",";
            }
        }
    // }
    // else{
    //     std::cout << std::string(p_ptr);
    // }
    free(p_ptr);
    std::cout << std::endl;
    return err;
}
// 查询内核信息
template <typename T>
void print_kernel_info(cl_kernel k, cl_kernel_info f, std::string name)
{
    cl_int err;
    size_t bs;
    
    err = clGetKernelInfo(k, f, 0, NULL, &bs);
    T* info = (T*)malloc(bs);
    err = clGetKernelInfo(k, f, bs, info, &bs);
    std::cout << name << " num " << bs / sizeof(T) << ":";
    for (size_t i = 0; i < bs / sizeof(T); i++){
        std::cout << info[i];
    }
    free(info);
    std::cout << std::endl;
}

// 创建上下文
void create_context(cl_platform_id platform)
{
    cl_int err;
    cl_device_id* devices;
    cl_uint num_devices;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
    devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
    // 选择gpu设备
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, num_devices, devices, NULL);
    // 创建上下文
    cl_context_properties properites[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
    // 指定上面的gpu设备创建上下文
    cl_context context = clCreateContext(properites, 1, devices, NULL, NULL, &err);
    if(err == CL_SUCCESS)
    {
        printf("Create Context success from platform with one gpu devices\n");
    }
    // 打印上下文信息
    print_context_info(context);
    // 创建命令队列-空
    cl_command_queue q = create_queue(context, devices[0]);
    // 创建程序对象
    std::vector<std::string> file_name;
    file_name.push_back("cl-kernel/add.cl");
    cl_program p1 = create_program(context, file_name);
    // 构建程序对象
    err = clBuildProgram(p1, 1, devices, "-cl-std=CL1.2 -cl-mad-enable -Werror", NULL, NULL);
    if(err == CL_SUCCESS)
    {
        printf("build program success\n");
    }
    print_program_info<char>(p1, CL_PROGRAM_SOURCE, "program source", true);
    print_program_info<size_t>(p1, CL_PROGRAM_NUM_KERNELS, " kernel num", false);
    print_program_info<char>(p1, CL_PROGRAM_KERNEL_NAMES, "kernel name", true);
    // 创建内核对象
    cl_kernel k1 = clCreateKernel(p1, "vector_add", &err);
    if(err == CL_SUCCESS){
        printf("build kernel success\n");
        print_kernel_info<char>(k1, CL_KERNEL_FUNCTION_NAME, "CL_KERNEL_FUNCTION_NAME");
        print_kernel_info<char>(k1, CL_KERNEL_ATTRIBUTES, "CL_KERNEL_ATTRIBUTES");
        print_kernel_info<cl_uint>(k1, CL_KERNEL_NUM_ARGS, "CL_KERNEL_NUM_ARGS");
        print_kernel_info<cl_uint>(k1, CL_KERNEL_REFERENCE_COUNT, "CL_KERNEL_REFERENCE_COUNT");
    }
    clReleaseProgram(p1);
    clUnloadPlatformCompiler(platform);
    clReleaseContext(context);
    free(devices);
    return;
    
}


int main()
{
    cl_platform_id *platform;
    cl_uint num_platform;
    cl_int err;
    err = clGetPlatformIDs(0,NULL, &num_platform);
    platform = (cl_platform_id *)malloc(sizeof (cl_platform_id) *
                                         num_platform);
    err = clGetPlatformIDs(num_platform,platform, NULL);
    for(int i = 0; i < num_platform; i++)
    {
        size_t size;
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_NAME,0,NULL,&size);
        char *PName = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_NAME,size,
                                PName,NULL);
        printf("\nCL_PLATFORM_NAME：% s\n",PName);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_VENDOR,
                                0,NULL,&size);
        char *PVendor = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_VENDOR,
                                size,PVendor, NULL);
        printf("CL_PLATFORM_VENDOR：% s\n", PVendor);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_VERSION,
                                0,NULL,&size);
        char *PVersion = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_VERSION,
                                size,PVersion, NULL);
        printf("CL_PLATFORM_VERSION：% s\n", PVersion);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_PROFILE,0,
                                NULL,&size);
        char *PProfile = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_PROFILE,
                                size,PProfile, NULL);
        printf("CL_PLATFORM_PROFILE：% s\n", PProfile);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_EXTENSIONS,
                                0,NULL,&size);
        char *PExten = (char *)malloc(size);
        err = clGetPlatformInfo(platform[i], CL_PLATFORM_EXTENSIONS,
                                size,PExten,NULL);
        printf("CL_PLATFORM_EXTENSIONS：% s\n", PExten);
        print_device(platform[i]);
        create_context(platform[i]);
        free(PName);
        free(PVendor);
        free(PVersion);
        free(PProfile);
        free(PExten);
    }
}