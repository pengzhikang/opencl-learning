#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <vector>
#include <cstring>
#include <opencv2/opencv.hpp>
#ifdef _APPLE_
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


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
/************************
*读取图像,返回图像对象
************************/
cl_mem LoadImage(cl_context context,char *fileName, int &width,
int &height)
{
    // FREE_IMAGE_FORMAT format =  FreeImage_GetFileType(fileName,0);
    // FIBITMAP *image = FreeImage_Load(format, fileName);
    // FIBITMAP *temp = image;
    // image = FreeImage_ConvertTo32Bits(image);
    // FreeImage_Unload(temp);
    // width = FreeImage_GetWidth(image);
    // height = FreeImage_GetHeight(image);
    cv::Mat img = cv::imread(std::string(fileName), 1);
    float rotio = std::min( 255.0 / img.cols, 255.0 / img.rows);
    int now_w = std::min(255, int(std::ceil(img.cols * rotio)));
    int now_h = std::min(255, int(std::ceil(img.rows * rotio)));
    cv::resize(img, img, cv::Size(now_w, now_h));
    cv::Mat org;
    cv::cvtColor(img, org, cv::COLOR_RGB2RGBA);
    // cv::imwrite("res1.png", org);
    width = org.cols;
    height = org.rows;
    char *buffer = new char[width * height * 4];
    memcpy(buffer, org.data ,width *  height * 4);
    cl_image_format clImageFormat;
    clImageFormat.image_channel_order = CL_RGBA;
    clImageFormat.image_channel_data_type =  CL_UNORM_INT8;
    cl_image_desc clImageDesc;
    memset(&clImageDesc,0,sizeof (cl_image_desc));
    clImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
    clImageDesc.image_width = width;
    clImageDesc.image_height = height;
    cl_int errNum;
    cl_mem clImage;
    clImage = clCreateImage(context,
                            CL_MEM_READ_ONLY |  CL_MEM_COPY_HOST_PTR,
                            &clImageFormat,
                            &clImageDesc,
                            buffer,&errNum);
    return clImage;
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

void check_code(cl_int errnum){
    switch (errnum)
    {
        case CL_INVALID_KERNEL_DEFINITION:
        case CL_INVALID_KERNEL:
            std::cout << "不合法的内核" << std::endl;
            return;
            break;
        case CL_INVALID_KERNEL_NAME:
            std::cout << "不合法的内核名字" << std::endl;
            return;
            break;
        case CL_INVALID_KERNEL_ARGS:
            std::cout << "内核的参数不合法" << std::endl;
            return;
            break;
        // case CL_FALSE:
        //     std::cout << "错误" << std::endl;
        //     return;
        //     break;
        // case CL_SUCCESS:
        //     std::cout << "正确" << std::endl;
        //     return;
        //     break;
        // case CL_INVALID_KERNEL:
        //     printf("invalid kernel\n");
        //     break;
        // case CL_INVALID_KERNEL:
        //     printf("invalid kernel\n");
        //     break;
        default:
            std::cout << "其他" << std::endl;
            return;
            break;
    }
    std::cout << "正常" << std::endl;
}

void GPU_Rotate(char *ImageFileName)
{
    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    cl_mem imageObjects[2] = { 0,0 };
    cl_int errNum;
    cl_platform_id platform;
    errNum = clGetPlatformIDs(1,&platform, NULL);
    errNum = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU,1,
                            &device,NULL);
    context = clCreateContext(NULL,1,&device, NULL,NULL,
                              &errNum);
    commandQueue = clCreateCommandQueue(context, device,NULL,
                                         &errNum);
    cl_bool if_support;
    size_t if_size = 0;
    clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, 0, NULL, &if_size);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, if_size, &if_support, &if_size);
    if (if_support == CL_TRUE){
        printf("mali-t860 support image sampler\n");
    }
    else{
        printf("mali-t860 does't support image sampler\n");
    }
    // get_device_info<size_t>(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, "cl_device_max_work_item_sizes");
    // get_device_info<cl_device_type>(device, CL_DEVICE_TYPE, "cl_device_type");
    // get_device_info<cl_uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, "cl_device_max_clock_frequency");
    // get_device_info<cl_ulong>(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "cl_device_global_mem_cacheline_size");
    // imageObjects[0]：原始图像;imageObjects[1]：原始 图像
    // cl_mem imageObjects[2] = { 0,0 };
    int width,height;
    imageObjects[0] = LoadImage(context, ImageFileName,width,
                                height);
    // 创建输出图像对象
    cl_image_format clImageFormat;
    clImageFormat.image_channel_order = CL_RGBA;
    clImageFormat.image_channel_data_type =  CL_UNORM_INT8;
    cl_image_desc desc;
    memset(&desc,'\0',sizeof(desc));
    desc.image_height = height;
    desc.image_width = width;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    imageObjects[1] = clCreateImage(context,CL_MEM_WRITE_ONLY,&clImageFormat,&desc,NULL,&errNum);
    assert(errNum == CL_SUCCESS);
    std::vector<std::string> pfile;
    pfile.push_back(std::string("rotate_image.cl"));
    program = create_program(context, pfile);
    errNum = clBuildProgram(program, 1, &device, "-cl-std=CL1.2 -cl-mad-enable -Werror", NULL, NULL);
    // std::cout << errNum << std::endl;
    // print_program_info<char>(program, CL_PROGRAM_SOURCE, "program source", true);
    kernel = clCreateKernel(program,"image_rotate", &errNum);
    // printf("%s", check_code(errNum).c_str());
    // std::cout << errNum << std::endl;
    // check_code(errNum);
    //assert(errNum == CL_SUCCESS);
    errNum = clSetKernelArg(kernel,0,sizeof (cl_mem),
                            &imageObjects[0]);
    errNum |= clSetKernelArg(kernel,1,sizeof (cl_mem),
                             &imageObjects[1]);
    float angle = 45 * std::acos(-1) / 180.0f;
    errNum |= clSetKernelArg(kernel,2,sizeof (cl_float),&angle);
    size_t globalWorkSize[2] =  {  width,height };
    errNum |= clEnqueueNDRangeKernel(commandQueue,
                                    kernel,2,NULL,
                                    globalWorkSize, NULL,
                                    0,NULL,NULL);
    assert(errNum == CL_SUCCESS);
    clFinish(commandQueue);
    // 拷贝旋转后的图像
    char *buffer = new char [width * height * 4];
    size_t origin[3] = { 0,0,0 };
    size_t region[3] = { width,height,1};
    errNum = clEnqueueReadImage(commandQueue,
                                imageObjects[1], CL_TRUE,
                                origin,region,0, 0,buffer,
                                0,NULL,NULL);
    assert(errNum == CL_SUCCESS);
    // double allv = 0;
    // for (size_t i = 0; i < width * height * 4; i++){
    //     allv += (buffer[i] / 255.0);
    // }
    // allv = 255 * allv / (width * height * 4);
    // printf("allv is %f\n", allv);
    // SaveImage("gpu_rotate.bmp",buffer,width, height);
    // Cleanup(context,commandQueue,program,kernel, imageObjects);
    cv::Mat res(height,width,  CV_8UC4, cv::Scalar(255, 0,0,255));
    memcpy(res.data, buffer, width * height * 4);
    cv::imwrite("result.png", res);
    clReleaseKernel(kernel);
    clReleaseMemObject(imageObjects[0]);
    clReleaseMemObject(imageObjects[1]);
    clReleaseProgram(program);
    clReleaseCommandQueue(commandQueue);
    clReleaseContext(context);
    clReleaseDevice(device);
}

int main()
{
    GPU_Rotate("org.jpg");
    return 0;
}