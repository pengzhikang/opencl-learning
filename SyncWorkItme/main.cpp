#include<CL/cl.h>
#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<iostream>
#include<string>
#include<assert.h>
template <typename T>
void get_device_info(cl_device_id device, cl_device_info param_name, std::string name)
{
	T* info = nullptr;
	size_t info_size = 0;
	clGetDeviceInfo(device, param_name, 0, NULL, &info_size);
	info = (T*)malloc(info_size / sizeof(T) + 1);
	std::cout << name << " info num " << info_size / sizeof(T) << ":";
	clGetDeviceInfo(device, param_name, info_size, info, &info_size);

	for (size_t i = 0; i < info_size / sizeof(T); i++)
	{
		std::cout << info[i] << ",";
	}
	//free((void*)info);
	std::cout << std::endl;
}

// 工作组间同步示例
void sync_opencl(cl_device_id device)
{
	cl_int errcode;
	cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &errcode);
	if (errcode != CL_SUCCESS || context == NULL)
	{
		printf("create context failed\n");
	}
	cl_command_queue command_queue = clCreateCommandQueue(context, device, 0, &errcode);
	if (errcode != CL_SUCCESS || command_queue == NULL)
	{
		printf("create command queue failed\n");
	}
    size_t wnum = 0;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, 0, NULL, &wnum);
    size_t wsize[wnum/sizeof(size_t)];
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, wnum, wsize, NULL);
    size_t allws = 1;
    for (int i = 0; i < 1; i++)
    {
        allws *= wsize[i];
    }
	const size_t contentLnegth = sizeof(int) * allws;
	cl_mem src1MemObj = clCreateBuffer(context, CL_MEM_READ_ONLY, contentLnegth, NULL, &errcode);
    assert(errcode == CL_SUCCESS);
	//cl_mem src2MemObj = clCreateBuffer(context, CL_MEM_READ_WRITE, contentLnegth, NULL, &errcode);
	//assert(errcode == CL_SUCCESS);
	int* pHostBuffer = (int*)malloc(contentLnegth);
	for (int i = 0; i < contentLnegth / sizeof(int); i++)
	{
		pHostBuffer[i] = i + 1;
	}
	errcode = clEnqueueWriteBuffer(command_queue, src1MemObj, CL_TRUE, 0, contentLnegth, pHostBuffer, 0, NULL, NULL);
	assert(errcode == CL_SUCCESS);
	const char* pFileName = "./kernel.cl";
	FILE* fp = fopen(pFileName, "r");
	assert(fp != NULL);
	fseek(fp, 0, SEEK_END);
	const long kernelLength = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* kernelSource = (char*)malloc(kernelLength + 1);
	fread(kernelSource, 1, kernelLength, fp);
	kernelSource[kernelLength] = '\0';
	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, (const size_t*)&kernelLength, &errcode);
	assert(errcode == CL_SUCCESS);
	size_t maxWorkGroupSize = 0;
	clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &maxWorkGroupSize, NULL);
	sprintf(kernelSource, "-D  GROUP_NUMBER_OF_WORKITEMS=%zu", maxWorkGroupSize);
	errcode = clBuildProgram(program, 1, &device, kernelSource, NULL, NULL);
    printf("%d,%d\n", kernelLength,errcode);
	assert(errcode == CL_SUCCESS);
	free(kernelSource);
	kernelSource = NULL;
	cl_kernel kernel = clCreateKernel(program, "kernel_test", &errcode);
	assert(errcode == CL_SUCCESS);
    printf("contentLnegth:%d\nmaxWorkGroupSize:%d\n", contentLnegth, maxWorkGroupSize);
	cl_mem src2MemObj = clCreateBuffer(context, CL_MEM_READ_WRITE, maxWorkGroupSize, NULL, &errcode);
	assert(errcode == CL_SUCCESS);
	cl_mem dstMemObj = clCreateBuffer(context, CL_MEM_READ_WRITE, maxWorkGroupSize * maxWorkGroupSize * sizeof(int), NULL, &errcode);
	assert(errcode == CL_SUCCESS);
	errcode |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &dstMemObj);
	errcode |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &src1MemObj);
	errcode |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &src2MemObj);
	assert(errcode == CL_SUCCESS);
	const size_t cs[] = { (size_t)contentLnegth / sizeof(int) };
	const size_t mw[] = { maxWorkGroupSize };
	errcode = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, cs, mw, 0, NULL, NULL);
	assert(errcode == CL_SUCCESS);
	clFinish(command_queue);
	int* pDeviceBuffer = (int*)malloc(contentLnegth);
	int remainCount;
	clEnqueueReadBuffer(command_queue, src2MemObj, CL_TRUE, 0, sizeof(int), &remainCount, 0, NULL, NULL);
	clEnqueueReadBuffer(command_queue, dstMemObj, CL_TRUE, 0, remainCount * sizeof(int), pDeviceBuffer, 0, NULL, NULL);
	int deviceSum = 0;
	for (int i = 0; i < remainCount; i++)
	{
		deviceSum += pDeviceBuffer[i];
	}
	int hostSum = 0;
	for (int i = 0; i < contentLnegth / sizeof(int); i++)
	{
		hostSum += pHostBuffer[i];
	}
	printf(deviceSum == hostSum ? "OK\n" : "NG\n");
	if (pHostBuffer != NULL)
		free(pHostBuffer);
	if (pDeviceBuffer != NULL)
		free(pDeviceBuffer);
	if (kernelSource != NULL)
		free(kernelSource);
	if (src1MemObj != NULL)
		clReleaseMemObject(src1MemObj);
	if (src2MemObj != NULL)
		clReleaseMemObject(src2MemObj);
	if (dstMemObj != NULL)
		clReleaseMemObject(dstMemObj);
	if (kernel != NULL)
		clReleaseKernel(kernel);
	if (program != NULL)
		clReleaseProgram(program);
	if (command_queue != NULL)
		clReleaseCommandQueue(command_queue);
	if (context != NULL)
		clReleaseContext(context);
}

void print_device(cl_platform_id platform)
{
	cl_int err;
	cl_device_id* devices;
	cl_uint num_devices;
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
	devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);
	for (size_t i = 0; i < num_devices; i++)
	{
		get_device_info<size_t>(devices[i], CL_DEVICE_MAX_WORK_ITEM_SIZES, "cl_device_max_work_item_sizes");
		get_device_info<cl_device_type>(devices[i], CL_DEVICE_TYPE, "cl_device_type");
		get_device_info<cl_uint>(devices[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, "cl_device_max_clock_frequency");
		get_device_info<cl_ulong>(devices[i], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "cl_device_global_mem_cacheline_size");
		sync_opencl(devices[i]);
	}
	free(devices);
}


int main()
{
	cl_platform_id *platform;
	cl_uint num_platform;
	cl_int err;
	err = clGetPlatformIDs(0, NULL, &num_platform);
	platform = (cl_platform_id *)malloc(sizeof(cl_platform_id) *
		num_platform);
	err = clGetPlatformIDs(num_platform, platform, NULL);
	for (int i = 0; i < num_platform; i++)
	{
		size_t size;
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_NAME, 0, NULL, &size);
		char *PName = (char *)malloc(size);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_NAME, size,
			PName, NULL);
		printf("\nCL_PLATFORM_NAME：% s\n", PName);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_VENDOR,
			0, NULL, &size);
		char *PVendor = (char *)malloc(size);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_VENDOR,
			size, PVendor, NULL);
		printf("CL_PLATFORM_VENDOR：% s\n", PVendor);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_VERSION,
			0, NULL, &size);
		char *PVersion = (char *)malloc(size);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_VERSION,
			size, PVersion, NULL);
		printf("CL_PLATFORM_VERSION：% s\n", PVersion);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_PROFILE, 0,
			NULL, &size);
		char *PProfile = (char *)malloc(size);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_PROFILE,
			size, PProfile, NULL);
		printf("CL_PLATFORM_PROFILE：% s\n", PProfile);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_EXTENSIONS,
			0, NULL, &size);
		char *PExten = (char *)malloc(size);
		err = clGetPlatformInfo(platform[i], CL_PLATFORM_EXTENSIONS,
			size, PExten, NULL);
		printf("CL_PLATFORM_EXTENSIONS：% s\n", PExten);
		print_device(platform[i]);
		free(PName);
		free(PVendor);
		free(PVersion);
		free(PProfile);
		free(PExten);
	}
}
