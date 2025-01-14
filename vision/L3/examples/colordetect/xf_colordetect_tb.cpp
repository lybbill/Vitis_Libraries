/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common/xf_headers.hpp"
#include "xcl2.hpp"
#include "xf_colordetect_tb_config.h"
void colordetect(cv::Mat& _src, cv::Mat& _dst, unsigned char* nLowThresh, unsigned char* nHighThresh) {
    // Temporary matrices for processing
    cv::Mat mask1, mask2, mask3, _imgrange, _imghsv;

    // Convert the input to the HSV colorspace. Using BGR here since it is the
    // default of OpenCV.
    // Using RGB yields different results, requiring a change of the threshold
    // ranges
    cv::cvtColor(_src, _imghsv, cv::COLOR_BGR2HSV);

    // Get the color of Yellow from the HSV image and store it as a mask
    cv::inRange(_imghsv, cv::Scalar(nLowThresh[0], nLowThresh[1], nLowThresh[2]),
                cv::Scalar(nHighThresh[0], nHighThresh[1], nHighThresh[2]), mask1);

    // Get the color of Green from the HSV image and store it as a mask
    cv::inRange(_imghsv, cv::Scalar(nLowThresh[3], nLowThresh[4], nLowThresh[5]),
                cv::Scalar(nHighThresh[3], nHighThresh[4], nHighThresh[5]), mask2);

    // Get the color of Red from the HSV image and store it as a mask
    cv::inRange(_imghsv, cv::Scalar(nLowThresh[6], nLowThresh[7], nLowThresh[8]),
                cv::Scalar(nHighThresh[6], nHighThresh[7], nHighThresh[8]), mask3);

    // Bitwise OR the masks together (adding them) to the range
    _imgrange = mask1 | mask2 | mask3;

    cv::Mat element = cv::getStructuringElement(0, cv::Size(3, 3), cv::Point(-1, -1));
    cv::erode(_imgrange, _dst, element, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT);

    cv::dilate(_dst, _dst, element, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT);

    cv::dilate(_dst, _dst, element, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT);

    cv::erode(_dst, _dst, element, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1> <INPUT IMAGE PATH 2>\n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img, img_rgba, out_img, ocv_ref, diff;

    // Open input image:
    in_img = cv::imread(argv[1], 1);
    if (!in_img.data) {
        fprintf(stderr, "ERROR: Could not open the input image.\n ");
        return -1;
    }

    // Allocate the memory for output images:
    out_img.create(in_img.rows, in_img.cols, CV_8UC1);
    ocv_ref.create(in_img.rows, in_img.cols, CV_8UC1);

    // Convert input from BGR to RGBA:
    //   cv::cvtColor(in_img, img_rgba, CV_BGR2RGBA);

    // Get processing kernel with desired shape - used in dilate and erode:

    cv::Mat element = cv::getStructuringElement(0, cv::Size(FILTER_SIZE, FILTER_SIZE), cv::Point(-1, -1));

    // Create vectors holding thresholds and shape:
    std::vector<unsigned char, aligned_allocator<unsigned char> > high_thresh(FILTER_SIZE * FILTER_SIZE);
    std::vector<unsigned char, aligned_allocator<unsigned char> > low_thresh(FILTER_SIZE * FILTER_SIZE);
    std::vector<unsigned char, aligned_allocator<unsigned char> > shape(FILTER_SIZE * FILTER_SIZE);

    for (int i = 0; i < (FILTER_SIZE * FILTER_SIZE); i++) {
        shape[i] = element.data[i];
    }

    // Define the low and high thresholds
    // Want to grab 3 colors (Yellow, Green, Red) for the input image
    low_thresh[0] = 22; // Lower boundary for Yellow
    low_thresh[1] = 150;
    low_thresh[2] = 60;

    high_thresh[0] = 38; // Upper boundary for Yellow
    high_thresh[1] = 255;
    high_thresh[2] = 255;

    low_thresh[3] = 38; // Lower boundary for Green
    low_thresh[4] = 150;
    low_thresh[5] = 60;

    high_thresh[3] = 75; // Upper boundary for Green
    high_thresh[4] = 255;
    high_thresh[5] = 255;

    low_thresh[6] = 160; // Lower boundary for Red
    low_thresh[7] = 150;
    low_thresh[8] = 60;

    high_thresh[6] = 179; // Upper boundary for Red
    high_thresh[7] = 255;
    high_thresh[8] = 255;

    int rows = in_img.rows;
    int cols = in_img.cols;
    std::cout << "Input image height : " << rows << std::endl;
    std::cout << "Input image width  : " << cols << std::endl;

    std::cout << "INFO: Thresholds loaded." << std::endl;

    // Start time for latency calculation of CPU function

    struct timespec begin_hw, end_hw, begin_cpu, end_cpu;
    clock_gettime(CLOCK_REALTIME, &begin_hw);

    // Reference function:
    colordetect(in_img, ocv_ref, low_thresh.data(), high_thresh.data());

    // End time for latency calculation of CPU function

    clock_gettime(CLOCK_REALTIME, &end_hw);
    long seconds, nanoseconds;
    double hw_time;

    seconds = end_hw.tv_sec - begin_hw.tv_sec;
    nanoseconds = end_hw.tv_nsec - begin_hw.tv_nsec;
    hw_time = seconds + nanoseconds * 1e-9;
    hw_time = hw_time * 1e3;

    // Write down reference and input image:
    cv::imwrite("outputref.png", ocv_ref);

    // OpenCL section:
    size_t image_in_size = in_img.rows * in_img.cols * in_img.channels() * sizeof(unsigned char);
    size_t image_out_size = in_img.rows * in_img.cols * sizeof(unsigned char);
    size_t vector_size = FILTER_SIZE * FILTER_SIZE * sizeof(unsigned char);

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    // Get the device:
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Contex, command queue and device name:
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, XF_NPPCX) << std::endl;
    std::cout << "NPPC:" << XF_NPPCX << std::endl;

    // Load binary:
    unsigned fileBufSize;
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_colordetect");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "color_detect", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size, NULL, &err));

    OCL_CHECK(err, cl::Buffer buffer_lThres(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, vector_size,
                                            low_thresh.data(), &err));

    OCL_CHECK(err, cl::Buffer buffer_hThres(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, vector_size,
                                            high_thresh.data(), &err));

    OCL_CHECK(err, cl::Buffer buffer_shapeKrnl(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, vector_size,
                                               shape.data(), &err));
    // printf("finished shape allocation\n");
    OCL_CHECK(err, cl::Buffer buffer_outImage(context, CL_MEM_WRITE_ONLY, image_out_size, NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_lThres));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_hThres));
    OCL_CHECK(err, err = kernel.setArg(3, buffer_shapeKrnl));
    OCL_CHECK(err, err = kernel.setArg(4, buffer_outImage));
    OCL_CHECK(err, err = kernel.setArg(5, rows));
    OCL_CHECK(err, err = kernel.setArg(6, cols));
    // printf("finished set arguments\n");

    // Intialize the buffers:
    cl::Event event;

    OCL_CHECK(err,
              queue.enqueueWriteBuffer(buffer_inImage, // buffer on the FPGA
                                       CL_TRUE,        // blocking call
                                       0,              // buffer offset in bytes
                                       image_in_size,  // Size in bytes
                                       in_img.data     // Pointer to the data to copy
                                       ));

    // Copy input data to device global memory
    OCL_CHECK(err, err = queue.enqueueMigrateMemObjects({buffer_lThres, buffer_hThres, buffer_shapeKrnl}, 0));

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel, NULL, &event));
    clWaitForEvents(1, (const cl_event*)&event);

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outImage, // This buffers data will be read
                            CL_TRUE,         // blocking call
                            0,               // offset
                            image_out_size,
                            out_img.data // Data will be stored here
                            );

    // Clean up:
    queue.finish();

    // Write down the kernel output image:
    cv::imwrite("output.png", out_img);

    // Results verification:
    int cnt = 0;
    cv::absdiff(ocv_ref, out_img, diff);
    cv::imwrite("diff.png", diff);

    for (int i = 0; i < diff.rows; ++i) {
        for (int j = 0; j < diff.cols; ++j) {
            uchar v = diff.at<uchar>(i, j);
            if (v > 0) cnt++;
        }
    }

    float err_per = 100.0 * (float)cnt / (diff.rows * diff.cols);

    std::cout << "INFO: Verification results:" << std::endl;
    std::cout << "\tPercentage of pixels above error threshold = " << err_per << "%" << std::endl;

    if (err_per > 0.0f) {
        fprintf(stderr, "ERROR: Test Failed.\n ");
        return EXIT_FAILURE;
    }

    std::cout.precision(3);
    std::cout << std::fixed;

    std::cout << "Latency for CPU function is: " << hw_time << "ms" << std::endl;

    return 0;
}
