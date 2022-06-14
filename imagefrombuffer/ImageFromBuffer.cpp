// Copyright (c) 2009-2014 Intel Corporation
// All rights reserved.
//
// WARRANTY DISCLAIMER
//
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly


#include <iostream>
#include <ctime>
#include <limits>
#include <cmath>

#include <CL/cl.h>

#include "basic.hpp"
#include "oclobject.hpp"
#include "utils.h"

using namespace std;

// According to the OpenCL 2.0 specification, you can create an image object can be created from a buffer object
// Such option is also available for OpenCL 1.2 devices that support the cl_khr_image2d_from_buffer extension
// Image From Buffer sample demonstrate how to create an OpenCL image based on the OpenCL buffer without extra coping.
// Once image is created you can use such image tools as interpolation and border checking.
int main (int argc, const char** argv)
{
    try
    {
        // OpenCL error code
        cl_int          err = 0;
        // width and height of the input image
        cl_int2         image_size;
        // number of pixels between lines for intermediate buffer
        int             intermediate_image_pitch;
        // OCL buffer for input and output data
        cl_mem          cl_inout_buffer = NULL;
        // OCL buffer for intermediate data
        cl_mem          cl_intermediate_buffer = NULL;
        // OCL image for intermediate data created based on cl_intermediate_buffer
        cl_mem          cl_intermediate_image = NULL;
        // 从.rgb文件中去读char输入数据
        vector<char>    input;
        readFile(L"../../images/ImageFromBuffer.rgb", input);
        image_size = *(cl_int2*)&input[0]; // input的前两个int数值是图片的长宽.


        // STEP 1: create device, context, queue, program, and kernels
        // Create the necessary OpenCL objects up to device queue.
        OpenCLBasic oclobjects("Intel","cpu");
        // Build programs and 2 kernels.
        OpenCLProgramMultipleKernels executable(oclobjects, L"../../ImageFromBuffer.cl", "");
        // kernels will be released automatically by the OpenCLProgramMultipleKernels destructor
        cl_kernel kernel_buffer = executable["process_buffer"];
        cl_kernel kernel_image = executable["process_image"];


        // STEP 2: OpenCL 1.2 device may support the image-from-buffer feature with the cl_khr_image2d_from_buffer extension
        //         So in this step we have to check if it is supported or not for OpenCL 1.2 devices
        //         Note, that OpenCL 2.0 device must support this feature and such check is not needed for 2.0 devices
        {
            const char* ext_name = "cl_khr_image2d_from_buffer";
            size_t len = 0;
            err = clGetDeviceInfo(
                oclobjects.device,
                CL_DEVICE_EXTENSIONS,
                NULL,
                NULL,
                &len);
            SAMPLE_CHECK_ERRORS(err);
            vector<char>    extentions(len);
            err = clGetDeviceInfo(
                oclobjects.device,
                CL_DEVICE_EXTENSIONS,
                len,
                &extentions[0],
                NULL
                );
            SAMPLE_CHECK_ERRORS(err);
            printf("Deivce extentions: %s\n", &extentions[0]);
            // extensions contain a space-separated list of extension names
            // to check that cl_khr_image2d_from_buffer is in the list, find
            // this sub string and be sure that it is ended by space or 0
            const char* ext_substr = strstr(&extentions[0],ext_name);
            const char* ext_substr_end = ext_substr+strlen(ext_name);
            if(!(ext_substr && (ext_substr_end[0] == ' ' ||  ext_substr_end[0] == 0 )))
            {// check that the device supports the image from buffer extension
                throw Error("The device does not support cl_khr_image2d_from_buffer extension");
            }
        }


        // STEP 3: get pitch for buffer to be able to use the image-from-buffer function
        {
            // required pitch alignment in pixels to be able to create an image object from a buffer object
            cl_uint pitch_alignment = 0;
            err = clGetDeviceInfo(
                oclobjects.device,
                CL_DEVICE_IMAGE_PITCH_ALIGNMENT,
                sizeof(pitch_alignment),
                &pitch_alignment,
                NULL);
            SAMPLE_CHECK_ERRORS(err);

            // calculate image pitch in pixels based on CL_DEVICE_IMAGE_PITCH_ALIGNMENT and image width
            // image pitch is calculated as the nearest value divided by pitch_aligment and greater or equal to the image width.
            intermediate_image_pitch = pitch_alignment * (1 + ((image_size.s[0] - 1) / pitch_alignment));
        }


        // STEP 4: allocate all buffers and handles
        {
            cl_image_format format; // format for intermediate image
            cl_image_desc   desc;   // description for intermediate image

            // create input-output buffer and copy RGBA data into it
            cl_inout_buffer = clCreateBuffer(
                oclobjects.context,
                CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR,
                image_size.s[0] * image_size.s[1]*sizeof(cl_float4),
                &input[sizeof(cl_int2)], // first cl_int2 are image sizes and we skip it.
                &err);
            SAMPLE_CHECK_ERRORS(err);

            // allocate regular OpenCL intermediate buffer
            // this buffer is output storage for the first OpenCL kernel
            // the content of this buffer is input for the second OpenCL kernel through cl_khr_image2d_from_buffer
            cl_intermediate_buffer = clCreateBuffer(
                oclobjects.context,
                CL_MEM_READ_WRITE,
                sizeof(cl_float4) * intermediate_image_pitch * image_size.s[1],
                NULL,
                &err);
            SAMPLE_CHECK_ERRORS(err);

            // create image from intermediate buffer
            // A 2D image cl_intermediate_image can be created from the cl_intermediate_buffer buffer
            // by specifying cl_intermediate_buffer in the image_desc->buffer field of cl_image_desc
            // When the contents of cl_intermediate_buffer data store are modified, those changes are reflected in the
            // contents of the cl_intermediate_image 2D image and vice-versa at corresponding synchronization points.

            // init image description
            memset(&desc,0,sizeof(desc));
            desc.image_type = CL_MEM_OBJECT_IMAGE2D;
            desc.image_width = image_size.s[0];
            desc.image_height = image_size.s[1];
            desc.image_row_pitch = intermediate_image_pitch * sizeof(cl_float4);
            // desc.mem_object = cl_intermediate_buffer;
            desc.buffer = cl_intermediate_buffer;

            // set image format as float RGBA
            format.image_channel_data_type = CL_FLOAT;
            format.image_channel_order = CL_RGBA;

            // create image from buffer
            // both image and buffer should use the same physical memory
            // So you don't need to copy them
            cl_intermediate_image = clCreateImage(
                oclobjects.context,
                CL_MEM_READ_WRITE,
                &format,
                &desc,
                NULL,
                &err);
            SAMPLE_CHECK_ERRORS(err);
        }

        
        // STEP 5. Set arguments for kernels and push them into the queue.
        {
            // set arguments for 1 kernel_buffer
            err  = clSetKernelArg(kernel_buffer, 0, sizeof(cl_mem), &cl_inout_buffer);
            SAMPLE_CHECK_ERRORS(err);
            err  = clSetKernelArg(kernel_buffer, 1, sizeof(cl_mem), &cl_intermediate_buffer);
            SAMPLE_CHECK_ERRORS(err);
            err  = clSetKernelArg(kernel_buffer, 2, sizeof(cl_int2), &image_size);
            SAMPLE_CHECK_ERRORS(err);
            err  = clSetKernelArg(kernel_buffer, 3, sizeof(cl_int), &intermediate_image_pitch);
            SAMPLE_CHECK_ERRORS(err);

            // submit first kernel that process buffer and make gamma correction to execute
            size_t globalsize[2] = {image_size.s[0],image_size.s[1]};
            err = clEnqueueNDRangeKernel(oclobjects.queue, kernel_buffer, 2, NULL, globalsize, NULL, 0, NULL, NULL);
            SAMPLE_CHECK_ERRORS(err);

            // set up the affine transform for image rotation around its centre
            float       angle = 0.3f;
            float       scale = 0.7f;
            // define and calculate the transformation matrix M and translation vector T
            // that will be used to calculate coordinate psrc of pixel in the source image
            // for a given coordinate pdst of pixel in destination image
            // psrc = M*psrc+T
            // calculate transformation matrix to produce rotation by angle and make scaling by scale parameters
            cl_float4   M = {
                 cos(angle)/scale, sin(angle)/scale,
                -sin(angle)/scale, cos(angle)/scale
            };
            // calculate translation vector to make rotation around centre of the image
            cl_float2   T = {
                0.5f*(image_size.s[0]-(M.s[0]*image_size.s[0]+M.s[1]*image_size.s[1])),
                0.5f*(image_size.s[1]-(M.s[2]*image_size.s[0]+M.s[3]*image_size.s[1]))
            };

            // Set arguments for kernel_image
            // The cl_intermediate_image image is input for this kernel. 
            // This image object shares the same memory as cl_intermediate_buffer buffer, that is output for the first kernel_buffer
            // So, we transparently pass output from the kernel_bufer into kernel_image without extra coping overhead from buffer object to image object.
            err  = clSetKernelArg(kernel_image, 0, sizeof(cl_mem), &cl_intermediate_image);
            SAMPLE_CHECK_ERRORS(err);
            err  = clSetKernelArg(kernel_image, 1, sizeof(cl_mem), &cl_inout_buffer);
            SAMPLE_CHECK_ERRORS(err);
            err  = clSetKernelArg(kernel_image, 2, sizeof(cl_int2), &image_size);
            SAMPLE_CHECK_ERRORS(err);
            err  = clSetKernelArg(kernel_image, 3, sizeof(cl_float4), &M);
            SAMPLE_CHECK_ERRORS(err);
            err  = clSetKernelArg(kernel_image, 4, sizeof(cl_float2), &T);
            SAMPLE_CHECK_ERRORS(err);

            // submit 2 kernel that processes image and makes geometric transform to execute
            err = clEnqueueNDRangeKernel(oclobjects.queue, kernel_image, 2, NULL, globalsize, NULL, 0, NULL, NULL);
            SAMPLE_CHECK_ERRORS(err);
        }


        // STEP 6: write final result into the file
        {
            // we have to call clEnqueueMapBuffer to access data on the host side
            void* ptr = clEnqueueMapBuffer(
                oclobjects.queue,
                cl_inout_buffer,
                CL_TRUE,    // blocking map
                CL_MAP_READ,
                0,
                intermediate_image_pitch * image_size.s[1]*sizeof(cl_float4),
                0, 0, 0,
                &err
                );
            SAMPLE_CHECK_ERRORS(err);

            // save bitmap with output image to show the final result
            SaveImageAsBMP_32FC4((cl_float*)ptr,255.0f,image_size.s[0],image_size.s[1],"../../result/ImageFromBufferOutput.bmp");

            err = clEnqueueUnmapMemObject(
                oclobjects.queue,
                cl_inout_buffer,
                ptr,
                0, 0, 0
                );
            SAMPLE_CHECK_ERRORS(err);

            // call clFinish to be sure that cl_inout_buffer is unmapped and we can safely release it
            err = clFinish(oclobjects.queue);
            SAMPLE_CHECK_ERRORS(err);

        }


        // release resources
        clReleaseMemObject(cl_inout_buffer);
        clReleaseMemObject(cl_intermediate_image);
        clReleaseMemObject(cl_intermediate_buffer);

        return EXIT_SUCCESS;
    }
    catch(const Error& error)
    {
        cerr << "[ ERROR ] Sample application specific error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    catch(const exception& error)
    {
        cerr << "[ ERROR ] " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    catch(...)
    {
        cerr << "[ ERROR ] Unknown/internal error happened.\n";
        return EXIT_FAILURE;
    }
}
