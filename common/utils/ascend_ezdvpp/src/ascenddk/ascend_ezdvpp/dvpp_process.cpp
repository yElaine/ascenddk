/**
 * ============================================================================
 *
 * Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#include <cstdlib>
#include <malloc.h>
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using namespace std;
namespace ascend {
namespace utils {
DvppProcess::DvppProcess(const DvppToJpgPara &para) {
    // construct a instance used to convert to JPG
    dvpp_instance_para_.jpg_para = para;
    convert_mode_ = kJpeg;
}

DvppProcess::DvppProcess(const DvppToH264Para &para) {
    // construct a instance used to convert to h264
    dvpp_instance_para_.h264_para = para;
    convert_mode_ = kH264;
}

DvppProcess::DvppProcess(const DvppToYuvPara &para) {
    // construct a instance used to convert to YUV420SPNV12
    dvpp_instance_para_.yuv_para = para;
    convert_mode_ = kYuv;
}

DvppProcess::DvppProcess(const DvppCropOrResizePara &para) {
    // construct a instance used to crop or resize image
    dvpp_instance_para_.crop_or_resize_para = para;
    convert_mode_ = kCropOrResize;
}

DvppProcess::DvppProcess(const DvppBasicVpcPara &para) {
    // construct a instance used to crop or resize image
    dvpp_instance_para_.basic_vpc_para = para;
    convert_mode_ = kBasicVpc;
}

DvppProcess::DvppProcess(const DvppJpegDInPara &para) {
    // construct a instance used to decode jpeg
    dvpp_instance_para_.jpegd_para = para;
    convert_mode_ = kJpegD;
}

ascend::utils::DvppProcess::~DvppProcess() {
    // destructor
}

int DvppProcess::DvppOperationProc(const char *input_buf, int input_size,
                                   DvppOutput *output_data) {
    int ret = kDvppOperationOk;
    DvppUtils dvpp_utils;

    // yuv change to h264
    if (convert_mode_ == kH264) {
        shared_ptr<AutoBuffer> output_data_queue;

        // process of coversion
        ret = DvppYuvChangeToH264(input_buf, input_size, &output_data_queue);
        if (ret != kDvppOperationOk) {
            return ret;
        }

        // check data size
        ret = dvpp_utils.CheckDataSize(output_data_queue->getBufferSize());
        if (ret != kDvppOperationOk) {
            ASC_LOG_ERROR(
                    "To prevent excessive memory, data size should be in "
                    "(0, 64]M! Now data size is %d byte.",
                    output_data_queue->getBufferSize());
            return ret;
        }

        // new output buffer
        output_data->buffer = new (nothrow) unsigned char[output_data_queue
                ->getBufferSize()];
        CHECK_NEW_RESULT(output_data->buffer);

        // output the h264 data
        output_data->size = output_data_queue->getBufferSize();
        ret = memcpy_s(output_data->buffer, output_data_queue->getBufferSize(),
                       output_data_queue->getBuffer(), output_data->size);
        CHECK_MEMCPY_RESULT(ret, output_data->buffer);  // if error,program exit
    } else if (convert_mode_ == kJpeg) {  // yuv change to jpg
        sJpegeOut jpg_output_data;

        // yuv change jpg
        ret = DvppYuvChangeToJpeg(input_buf, input_size, &jpg_output_data);
        if (ret != kDvppOperationOk) {
            return ret;
        }

        // check data size
        ret = dvpp_utils.CheckDataSize(jpg_output_data.jpgSize);
        if (ret != kDvppOperationOk) {
            ASC_LOG_ERROR(
                    "To prevent excessive memory, data size should be in "
                    "(0, 64]M!, Now data size is %d byte.",
                    jpg_output_data.jpgSize);
            return ret;
        }

        // new output buffer
        output_data->buffer = new (nothrow) unsigned char[jpg_output_data
                .jpgSize];
        CHECK_NEW_RESULT(output_data->buffer);

        // output the jpg data
        output_data->size = jpg_output_data.jpgSize;
        ret = memcpy_s(output_data->buffer, output_data->size,
                       jpg_output_data.jpgData, jpg_output_data.jpgSize);
        jpg_output_data.cbFree();
        CHECK_MEMCPY_RESULT(ret, output_data->buffer);  // if error,program exit
    } else if (convert_mode_ == kYuv) {  // bgr change to yuv
        // the size of output buffer
        int data_size = dvpp_instance_para_.yuv_para.resolution.width
                * dvpp_instance_para_.yuv_para.resolution.height *
                DVPP_YUV420SP_SIZE_MOLECULE /
        DVPP_YUV420SP_SIZE_DENOMINATOR;

        // check data size
        ret = dvpp_utils.CheckDataSize(data_size);
        if (ret != kDvppOperationOk) {
            ASC_LOG_ERROR(
                    "To prevent excessive memory, data size should be in "
                    "(0, 64]M! Now data size is %d byte. width is %d, "
                    "height is %d.",
                    data_size, dvpp_instance_para_.yuv_para.resolution.width,
                    dvpp_instance_para_.yuv_para.resolution.height);
            return ret;
        }

        // new output buffer
        unsigned char *yuv_output_data = new (nothrow) unsigned char[data_size];
        CHECK_NEW_RESULT(yuv_output_data);

        // bgr change to yuv420spnv12
        ret = DvppBgrChangeToYuv(input_buf, input_size, data_size,
                                 yuv_output_data);
        if (ret != kDvppOperationOk) {
            delete[] yuv_output_data;
            return ret;
        }

        // output the nv12 data
        output_data->buffer = yuv_output_data;
    } else if (convert_mode_ == kCropOrResize) {  // crop or resize image

        // set width of dest image
        int dest_width = dvpp_instance_para_.crop_or_resize_para.dest_resolution
                .width;

        // set height of dest image
        int dest_high = dvpp_instance_para_.crop_or_resize_para.dest_resolution
                .height;

        int data_size = 0;

        // If output image need alignment, the memory size is calculated after
        // width and height alignment
        if (dvpp_instance_para_.crop_or_resize_para.is_output_align) {
            data_size = ALIGN_UP(dest_width, kVpcWidthAlign)
                    * ALIGN_UP(dest_high, kVpcHeightAlign) *
                    DVPP_YUV420SP_SIZE_MOLECULE /
            DVPP_YUV420SP_SIZE_DENOMINATOR;
        } else {  // output image does not need alignment
            data_size = dest_width * dest_high *
            DVPP_YUV420SP_SIZE_MOLECULE /
            DVPP_YUV420SP_SIZE_DENOMINATOR;
        }

        // check data size
        ret = dvpp_utils.CheckDataSize(data_size);
        if (ret != kDvppOperationOk) {
            ASC_LOG_ERROR(
                    "To prevent excessive memory, data size should be in "
                    "(0, 64]M! Now data size is %d byte. width is %d, "
                    "height is %d.",
                    data_size, dest_width, dest_high);
            return ret;
        }

        // create output buffer
        unsigned char *output_buffer = new (nothrow) unsigned char[data_size];
        CHECK_NEW_RESULT(output_buffer);

        //crop or resize image
        ret = DvppCropOrResize(input_buf, input_size, data_size, output_buffer);
        if (ret != kDvppOperationOk) {
            delete[] output_buffer;
            return ret;
        }

        // output the nv12 data
        output_data->buffer = output_buffer;
        output_data->size = data_size;
    }
    return ret;
}

int DvppProcess::DvppJpegDProc(const char *input_buf, int input_size,
                               DvppJpegDOutput *output_data) {
    int ret = kDvppOperationOk;
    jpegd_yuv_data_info jpegd_out;

    // jpeg decode to yuv
    ret = DvppJpegChangeToYuv(input_buf, input_size, &jpegd_out);
    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR("Failed to call DvppJpegDProc.");
        return ret;
    }

    DvppUtils dvpp_utils;
    // check jpegd_out.yuv_data_size parameters
    ret = dvpp_utils.CheckDataSize(jpegd_out.yuv_data_size);
    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "To prevent excessive memory, data size should be in (0, 64]M! "
                "Now data size is %d byte.",
                jpegd_out.yuv_data_size);
        return ret;
    }

    // construct output data
    output_data->buffer = new (nothrow) unsigned char[jpegd_out.yuv_data_size];
    CHECK_NEW_RESULT(output_data->buffer);

    output_data->buffer_size = jpegd_out.yuv_data_size;
    output_data->width = jpegd_out.img_width;
    output_data->height = jpegd_out.img_height;
    output_data->aligned_width = jpegd_out.img_width_aligned;
    output_data->aligned_height = jpegd_out.img_height_aligned;

    // Format conversion, the output format of jpegD interface is specifically
    // formatted into the vpc interface format. After transformation, V of yuv
    // image is in front of u.
    switch (jpegd_out.out_format) {
        case (DVPP_JPEG_DECODE_OUT_YUV444):
            // yuv444sp
            output_data->image_format = INPUT_YUV444_SEMI_PLANNER_VU;
            break;
        case (DVPP_JPEG_DECODE_OUT_YUV422_H2V1):
            // yuv422sp
            output_data->image_format = INPUT_YUV422_SEMI_PLANNER_VU;
            break;
        case (DVPP_JPEG_DECODE_OUT_YUV420):
            // yuv420sp
            output_data->image_format = INPUT_YUV420_SEMI_PLANNER_VU;
            break;
        case (DVPP_JPEG_DECODE_OUT_YUV400):
            // yuv400sp, the subsequent processing remains the same as 420sp.
            output_data->image_format = INPUT_YUV420_SEMI_PLANNER_VU;
            break;
        default:
            ASC_LOG_ERROR(
                    "[jpeg decode]The current image format is not supported");
            ret = kDvppErrorInvalidParameter;
            break;
    }

    if (ret != kDvppOperationOk) {
        return ret;
    }

    // output yuv data
    ret = memcpy_s(output_data->buffer, output_data->buffer_size,
                   jpegd_out.yuv_data, jpegd_out.yuv_data_size);

    // free memory
    jpegd_out.cbFree();
    CHECK_MEMCPY_RESULT(ret, output_data->buffer);
    return ret;
}

int DvppProcess::DvppBasicVpcProc(const uint8_t *input_buf, int32_t input_size,
                                  DvppVpcOutput *output_data) {
    int ret = kDvppOperationOk;

    // set width of dest image
    int dest_width = dvpp_instance_para_.basic_vpc_para.dest_resolution.width;

    // set height of dest image
    int dest_high = dvpp_instance_para_.basic_vpc_para.dest_resolution.height;

    int data_size = 0;

    //If output image need alignment, the memory size is calculated after width
    // and height alignment
    if (dvpp_instance_para_.basic_vpc_para.is_output_align) {
        data_size = ALIGN_UP(dest_width, kVpcWidthAlign)
                * ALIGN_UP(dest_high, kVpcHeightAlign) *
                DVPP_YUV420SP_SIZE_MOLECULE /
        DVPP_YUV420SP_SIZE_DENOMINATOR;
    } else {  // output image does not need alignment
        data_size = dest_width * dest_high *
        DVPP_YUV420SP_SIZE_MOLECULE /
        DVPP_YUV420SP_SIZE_DENOMINATOR;
    }

    DvppUtils dvpp_utils;
    // check data size
    ret = dvpp_utils.CheckDataSize(data_size);
    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "To prevent excessive memory, data size should be in (0, 64]M! "
                "Now data size is %d byte. width is %d, height is %d.",
                data_size, dest_width, dest_high);
        return ret;
    }

    // create output buffer
    uint8_t *output_buffer = new (nothrow) unsigned char[data_size];
    CHECK_NEW_RESULT(output_buffer);

    //crop or resize image
    ret = DvppBasicVpc(input_buf, input_size, data_size, output_buffer);
    if (ret != kDvppOperationOk) {
        delete[] output_buffer;
        return ret;
    }

    // output the nv12 data
    output_data->buffer = output_buffer;
    output_data->size = data_size;

    return ret;
}

int DvppProcess::DvppProc(const sJpegeIn &input_data, sJpegeOut *output_data) {
    if (output_data == nullptr) {
        return kDvppErrorInvalidParameter;
    }

    dvppapi_ctl_msg dvpp_api_ctl_msg;
    dvpp_api_ctl_msg.in = (void *) &input_data;
    dvpp_api_ctl_msg.in_size = sizeof(input_data);
    dvpp_api_ctl_msg.out = (void *) output_data;
    dvpp_api_ctl_msg.out_size = sizeof(sJpegeOut);

    // create Dvpp instance
    IDVPPAPI *dvpp_api = nullptr;
    int ret = CreateDvppApi(dvpp_api);
    if ((dvpp_api == nullptr) || (ret == kDvppReturnError)) {
        ret = kDvppErrorCreateDvppFail;
        ASC_LOG_ERROR("Failed to create instance of dvpp(yuv to jpeg).");
        return ret;
    }

    // convert
    ret = DvppCtl(dvpp_api, DVPP_CTL_JPEGE_PROC, &dvpp_api_ctl_msg);
    if (ret == kDvppReturnError) {
        ret = kDvppErrorDvppCtlFail;
        ASC_LOG_ERROR("Failed to convert in dvpp(yuv to jpeg).");
    }

    // destroy Dvpp instance
    (void) DestroyDvppApi(dvpp_api);

    return ret;
}

int DvppProcess::DvppYuvChangeToJpeg(const char *input_buf, int input_size,
                                     sJpegeOut *output_data) {
    sJpegeIn input_data;
    int ret = kDvppOperationOk;

    if ((input_buf == nullptr) || (input_size <= 0)
            || (output_data == nullptr)) {
        ASC_LOG_ERROR(
                "The input parameter is error in dvpp(yuv to jpeg)."
                "parameter is pInput_buf:%d input_size:%d pOut_data:%d.",
                input_buf, input_size, output_data);
        return kDvppErrorInvalidParameter;
    }

    // yuv image width/height/encoding quality level(1-100)/format/height
    // after aligned
    input_data.width = dvpp_instance_para_.jpg_para.resolution.width;
    input_data.height = dvpp_instance_para_.jpg_para.resolution.height;
    input_data.level = dvpp_instance_para_.jpg_para.level;
    input_data.format = (eEncodeFormat) dvpp_instance_para_.jpg_para.format;
    input_data.heightAligned = dvpp_instance_para_.jpg_para.resolution.height;

    if (JPGENC_FORMAT_YUV420 == (input_data.format & JPGENC_FORMAT_BIT)) {
        // bufsize calculate according to  stride and heightAligned.
        // bufsize finally align height to 4k
        if (!dvpp_instance_para_.jpg_para.is_align_image) {
            // align width to 16
            input_data.stride = ALIGN_UP(input_data.width, kJpegEWidthAlgin);

            input_data.bufSize =
                    ALIGN_UP(
                            input_data.stride * input_data.heightAligned *
                            DVPP_YUV420SP_SIZE_MOLECULE /
                            DVPP_YUV420SP_SIZE_DENOMINATOR,
                            PAGE_SIZE);
        } else {
            input_data.stride = ALIGN_UP(input_data.width,
                                         kJpegECompatWidthAlign);
            input_data.heightAligned = ALIGN_UP(
                    dvpp_instance_para_.jpg_para.resolution.height,
                    kJpegEHeightAlign);
            input_data.bufSize =
                    ALIGN_UP(
                            input_data.stride * input_data.heightAligned *
                            DVPP_YUV420SP_SIZE_MOLECULE /
                            DVPP_YUV420SP_SIZE_DENOMINATOR,
                            PAGE_SIZE);
        }
    }

    unsigned int mmap_size = ALIGN_UP(input_data.bufSize + kJpegEAddressAlgin,
                                      MAP_2M);

    // apply for memory: 1.Large-page
    unsigned char* addr_orig = (unsigned char*) mmap(
        0, mmap_size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | API_MAP_VA32BIT, -1, 0);

    if (addr_orig == MAP_FAILED) {
      ASC_LOG_INFO(
          "Failed to malloc memory in dvpp(yuv to jpeg), start to try 4K "
          "memory");
      addr_orig = (unsigned char*) mmap(
          0, mmap_size, PROT_READ | PROT_WRITE,
          MAP_PRIVATE | MAP_ANONYMOUS | API_MAP_VA32BIT, -1, 0);
      if (addr_orig == MAP_FAILED) {
        ASC_LOG_ERROR("4K memory malloc still fail.");
        return kDvppErrorMallocFail;
      }
    }

    // first address of buffer align to 128
    input_data.buf = (unsigned char*) ALIGN_UP((uint64_t ) addr_orig,
                                               kJpegEAddressAlgin);

    const char *temp_buf = nullptr;

    // input data align to specify requirement
    if (JPGENC_FORMAT_YUV420 == (input_data.format & JPGENC_FORMAT_BIT)) {
        temp_buf = input_buf;
        if (dvpp_instance_para_.jpg_para.is_align_image) {
            memcpy_s(input_data.buf, mmap_size, temp_buf, input_size);
            CHECK_MEMCPY_RESULT(ret, nullptr);  // if exist error,program exit
        } else {
            for (unsigned int j = 0; j < input_data.height; j++) {
                ret = memcpy_s(
                        input_data.buf + ((ptrdiff_t) j * input_data.stride),
                        (unsigned) (mmap_size - j * input_data.stride),
                        temp_buf, (unsigned) (input_data.width));
                CHECK_MEMCPY_RESULT(ret, nullptr); // if error,program exit
                temp_buf += input_data.width;
            }
            for (unsigned int j = input_data.heightAligned;
                    j < input_data.heightAligned + input_data.height / 2; j++) {
                ret = memcpy_s(
                        input_data.buf + ((ptrdiff_t) j * input_data.stride),
                        (unsigned) (mmap_size - j * input_data.stride),
                        temp_buf, (unsigned) (input_data.width));
                CHECK_MEMCPY_RESULT(ret, nullptr); // if error,program exit
                temp_buf += input_data.width;
            }
        }
    }

    // call dvpp
    ret = DvppProc(input_data, output_data);

    // release buffer
    if (addr_orig != MAP_FAILED) {
        munmap(addr_orig,
               (unsigned) (ALIGN_UP(input_data.bufSize + kJpegEAddressAlgin,
                                    MAP_2M)));
    }

    return ret;
}

int DvppProcess::DvppYuvChangeToH264(const char *input_buf, int input_size,
                                     shared_ptr<AutoBuffer> *output_buf) {
    // YUV width/height/coding protocol/stroage type
    venc_in_msg venc_msg;
    venc_msg.width = dvpp_instance_para_.h264_para.resolution.width;
    venc_msg.height = dvpp_instance_para_.h264_para.resolution.height;
    venc_msg.coding_type = dvpp_instance_para_.h264_para.coding_type;
    venc_msg.YUV_store_type = dvpp_instance_para_.h264_para.yuv_store_type;
    venc_msg.input_data = const_cast<char*>(input_buf);
    venc_msg.input_data_size = input_size;
    venc_msg.output_data_queue = make_shared<AutoBuffer>();

    dvppapi_ctl_msg dvpp_api_ctl_msg;
    dvpp_api_ctl_msg.in = (void*) (&venc_msg);
    dvpp_api_ctl_msg.in_size = sizeof(venc_in_msg);

    // create Dvpp instance
    IDVPPAPI *dvpp_api = nullptr;
    int ret = CreateDvppApi(dvpp_api);

    // success to create a dvpp instance.
    if ((dvpp_api != nullptr) && (ret != kDvppReturnError)) {
        // convert
        if (DvppCtl(dvpp_api, DVPP_CTL_VENC_PROC, &dvpp_api_ctl_msg)
                == kDvppReturnError) {
            // failed to convert.
            ret = kDvppErrorDvppCtlFail;
            ASC_LOG_ERROR("Failed to convert in dvpp(yuv to h264).");
            DestroyDvppApi(dvpp_api);
            return ret;
        }

        // check the output date.
        if ((dvpp_api_ctl_msg.in == nullptr)
                || ((dvpp_api_ctl_msg.in != nullptr)
                        && (((venc_in_msg *) (dvpp_api_ctl_msg.in))
                                ->output_data_queue == nullptr))) {
            ret = kDvppErrorNoOutputInfo;
            ASC_LOG_ERROR("Failed to get data in dvpp(yuv to h264).");
            DestroyDvppApi(dvpp_api);
            return ret;
        }

        *output_buf = venc_msg.output_data_queue;
    } else {  // failed to create dvpp.
        ASC_LOG_ERROR("Failed to create instance of dvpp(yuv to h264).");
        return kDvppErrorCreateDvppFail;
    }

    // destroy Dvpp instance
    DestroyDvppApi(dvpp_api);

    return ret;
}

int DvppProcess::GetMode() const {
    return convert_mode_;
}

void DvppProcess::PrintErrorInfo(int code) const {

    static ErrorDescription dvpp_description[] = { { kDvppErrorInvalidParameter,
            "The parameter is invaild." }, { kDvppErrorMallocFail,
            "Failed to malloc memory in dvpp." }, { kDvppErrorCreateDvppFail,
            "Failed to create dvpp." }, { kDvppErrorDvppCtlFail,
            "Failed to operate dvpp." }, { kDvppErrorNoOutputInfo,
            "The dvpp output is no data." }, { kDvppErrorMemcpyFail,
            "Failed to copy info." }, };

    // find same errorcode and get error description
    int num = sizeof(dvpp_description) / sizeof(ErrorDescription);

    for (int i = 0; i < num; i++) {
        if (code == dvpp_description[i].code) {
            cerr << "[ERROR] " << dvpp_description[i].code_info.c_str() << endl;
            return;
        }
    }

    // not found same errorcode,return
    cerr << "[ERROR] Other error." << endl;
}

int DvppProcess::DvppBgrChangeToYuv(const char *input_buf, int input_size,
                                    int output_size,
                                    unsigned char *output_buf) {
    DvppUtils dvpp_utils;

    // check input & output param
    int ret = dvpp_utils.CheckBgrToYuvParam(input_buf, input_size, output_size,
                                            output_buf);

    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR("Input param and output param can not be null!");
        return ret;
    }

    // construct vpc parameters
    dvppapi_ctl_msg dvpp_api_ctl_msg;
    vpc_in_msg vpc_in_msg;

    vpc_in_msg.format = dvpp_instance_para_.yuv_para.image_type;
    vpc_in_msg.cvdr_or_rdma = dvpp_instance_para_.yuv_para.cvdr_or_rdma;
    vpc_in_msg.bitwidth = dvpp_instance_para_.yuv_para.bit_width;
    vpc_in_msg.rank = dvpp_instance_para_.yuv_para.rank;
    vpc_in_msg.width = dvpp_instance_para_.yuv_para.resolution.width;
    vpc_in_msg.high = dvpp_instance_para_.yuv_para.resolution.height;
    vpc_in_msg.hmax = dvpp_instance_para_.yuv_para.horz_max;
    vpc_in_msg.hmin = dvpp_instance_para_.yuv_para.horz_min;
    vpc_in_msg.vmax = dvpp_instance_para_.yuv_para.vert_max;
    vpc_in_msg.vmin = dvpp_instance_para_.yuv_para.vert_min;
    vpc_in_msg.vinc = dvpp_instance_para_.yuv_para.vert_inc;
    vpc_in_msg.hinc = dvpp_instance_para_.yuv_para.horz_inc;

    // 1 pixel in the BGR image occupies 3 bytes, so the application space
    // is 3 times the number of pixels.
    int rgb_width = vpc_in_msg.width * DVPP_BGR_BUFFER_MULTIPLE;

    // 128 byte memory alignment in width direction of bgr image
    vpc_in_msg.stride = ALIGN_UP(rgb_width, kVpcWidthAlign);

    // 16 byte memory alignment in height direction of bgr image
    int high_align = ALIGN_UP(vpc_in_msg.high, kVpcHeightAlign);

    // total storage size of bgr image after the memory is aligned
    int in_buffer_size = vpc_in_msg.stride * high_align;

    // input data address 128 byte alignment
    char *in_buffer = (char *) memalign(kVpcAddressAlign, in_buffer_size);
    CHECK_NEW_RESULT(in_buffer);

    // check image whether need to align
    int image_align = kImageNeedAlign;
    image_align = dvpp_utils.CheckImageNeedAlign(vpc_in_msg.width,
                                                 vpc_in_msg.high);

    // If original image is already memory aligned, directly copy all memory.
    if (image_align == kImageNotNeedAlign) {
        ret = memcpy_s(in_buffer, in_buffer_size, input_buf, input_size);
        CHECK_VPC_MEMCPY_S_RESULT(ret, in_buffer, nullptr);
    } else {  //If image is not aligned, memory copy from line to line.
        // remain memory size in input buffer
        int remain_in_buffer_size = in_buffer_size;

        for (int i = 0; i < vpc_in_msg.high; ++i) {
            ret = memcpy_s(in_buffer + ((ptrdiff_t) i * vpc_in_msg.stride),
                           remain_in_buffer_size, input_buf, rgb_width);
            CHECK_VPC_MEMCPY_S_RESULT(ret, in_buffer, nullptr);
            //Point the pointer to next row of data
            input_buf += rgb_width;
            remain_in_buffer_size -= vpc_in_msg.stride;
        }
    }

    vpc_in_msg.in_buffer = in_buffer;
    vpc_in_msg.in_buffer_size = in_buffer_size;

    shared_ptr<AutoBuffer> auto_out_buffer = make_shared<AutoBuffer>();
    vpc_in_msg.auto_out_buffer_1 = auto_out_buffer;
    dvpp_api_ctl_msg.in = (void *) (&vpc_in_msg);
    dvpp_api_ctl_msg.in_size = sizeof(vpc_in_msg);

    // Create DVPP API
    IDVPPAPI *dvpp_api = nullptr;
    ret = CreateDvppApi(dvpp_api);

    // call DVPP VPC interface
    if (dvpp_api != nullptr && ret == kDvppReturnOk) {
        if (DvppCtl(dvpp_api, DVPP_CTL_VPC_PROC, &dvpp_api_ctl_msg)
                != kDvppOperationOk) {
            ASC_LOG_ERROR("call dvppctl process faild!");
            DestroyDvppApi(dvpp_api);
            free(in_buffer);
            return kDvppErrorDvppCtlFail;
        }
    } else {  // create dvpp api fail, directly return
        ASC_LOG_ERROR("piDvppApi is null!");
        DestroyDvppApi(dvpp_api);
        free(in_buffer);
        return kDvppErrorCreateDvppFail;
    }

    // 128 byte memory alignment in width direction of yuv image
    int yuv_stride = ALIGN_UP(vpc_in_msg.width, kVpcWidthAlign);

    int out_buffer_size = vpc_in_msg.width * vpc_in_msg.high *
    DVPP_YUV420SP_SIZE_MOLECULE /
    DVPP_YUV420SP_SIZE_DENOMINATOR;

    // The output image is also memory aligned, so if original image is already
    // memory aligned, directly copy all memory.
    if (image_align == kImageNotNeedAlign) {
        ret = memcpy_s(output_buf, out_buffer_size,
                       vpc_in_msg.auto_out_buffer_1->getBuffer(),
                       vpc_in_msg.auto_out_buffer_1->getBufferSize());
        CHECK_VPC_MEMCPY_S_RESULT(ret, in_buffer, dvpp_api);
    } else {  // If image is not aligned, memory copy from line to line.
        // dvpp output buffer
        char *vpc_out_buffer = vpc_in_msg.auto_out_buffer_1->getBuffer();

        // DvppBgrChangeToYuv output buffer index, like array indexes,
        // start at 0
        int out_index = 0;

        // remain memory size in DvppBgrChangeToYuv output buffer
        int remain_out_buffer_size = out_buffer_size;

        // y channel data copy
        for (int j = 0; j < vpc_in_msg.high; ++j) {
            ret = memcpy_s(
                    output_buf + (ptrdiff_t) out_index * vpc_in_msg.width,
                    remain_out_buffer_size, vpc_out_buffer, vpc_in_msg.width);
            CHECK_VPC_MEMCPY_S_RESULT(ret, in_buffer, dvpp_api);

            // Point the pointer to next row of data
            vpc_out_buffer += yuv_stride;
            out_index++;
            remain_out_buffer_size -= vpc_in_msg.width;
        }

        // uv channel data copy
        vpc_out_buffer += (ptrdiff_t) (high_align - vpc_in_msg.high)
                * yuv_stride;

        for (int k = high_align; k < high_align + vpc_in_msg.high / 2; ++k) {
            ret = memcpy_s(
                    output_buf + (ptrdiff_t) out_index * vpc_in_msg.width,
                    remain_out_buffer_size, vpc_out_buffer, vpc_in_msg.width);
            CHECK_VPC_MEMCPY_S_RESULT(ret, in_buffer, dvpp_api);

            // Point the pointer to next row of data
            vpc_out_buffer += yuv_stride;
            out_index++;
            remain_out_buffer_size -= vpc_in_msg.width;
        }
    }

    // free memory
    free(in_buffer);
    DestroyDvppApi(dvpp_api);

    return ret;
}

int DvppProcess::DvppCropOrResize(const char *input_buf, int input_size,
                                  int output_size, unsigned char *output_buf) {
    DvppUtils dvpp_utils;

    // check input and output param
    int ret = dvpp_utils.CheckCropOrResizeParam(input_buf, input_size,
                                                output_size, output_buf);

    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "Crop or resize input param and output param can not be null!");
        return ret;
    }

    // When using VPC for image cropping and resizing, it is necessary to call
    // two times DvppCtrl: the first call, the input parameter is
    // resize_param_in_msg and the output parameter is resize_param_out_msg;
    // the second call, the input parameter is vpc_in_msg, and the output
    // parameter is vpc_out_msg
    // step 1. First call VPC interface
    // construct resize parameters
    vpc_in_msg vpc_in_msg;
    resize_param_in_msg resize_in_param;
    resize_param_out_msg resize_out_param;

    // Input width and height must be even. If not, convert to even.
    int even_width = (dvpp_instance_para_.crop_or_resize_para.src_resolution
            .width >> 1) << 1;
    int even_high = (dvpp_instance_para_.crop_or_resize_para.src_resolution
            .height >> 1) << 1;

    // width need 128-byte alignment
    resize_in_param.src_width = ALIGN_UP(even_width, kVpcWidthAlign);

    // height need 16-byte alignment
    resize_in_param.src_high = ALIGN_UP(even_high, kVpcHeightAlign);

    // The maximum deviation from the origin in horz direction
    resize_in_param.hmax = dvpp_instance_para_.crop_or_resize_para.horz_max;

    // The minimum deviation from the origin in horz direction
    resize_in_param.hmin = dvpp_instance_para_.crop_or_resize_para.horz_min;

    // The maximum deviation from the origin in vert direction
    resize_in_param.vmax = dvpp_instance_para_.crop_or_resize_para.vert_max;

    // The minimum deviation from the origin in vert direction
    resize_in_param.vmin = dvpp_instance_para_.crop_or_resize_para.vert_min;

    // Image width after crop or resize
    resize_in_param.dest_width = dvpp_instance_para_.crop_or_resize_para
            .dest_resolution.width;

    // Image height after crop or resize
    resize_in_param.dest_high = dvpp_instance_para_.crop_or_resize_para
            .dest_resolution.height;

    dvppapi_ctl_msg dvpp_api_ctl_msg;
    dvpp_api_ctl_msg.in = (void *) (&resize_in_param);

    dvpp_api_ctl_msg.out = (void *) (&resize_out_param);

    // create dvpp api
    IDVPPAPI *pi_dvpp_api = nullptr;
    ret = CreateDvppApi(pi_dvpp_api);

    // call DVPP VPC interface
    if (pi_dvpp_api != nullptr && ret == kDvppReturnOk) {
        if (DvppCtl(pi_dvpp_api, DVPP_CTL_TOOL_CASE_GET_RESIZE_PARAM,
                    &dvpp_api_ctl_msg) != kDvppOperationOk) {
            ASC_LOG_ERROR("call dvppctl process faild!");
            DestroyDvppApi(pi_dvpp_api);
            return kDvppErrorDvppCtlFail;
        }
    } else {  // create dvpp api fail, directly return
        ASC_LOG_ERROR("pi_dvpp_api is null!");
        DestroyDvppApi(pi_dvpp_api);
        return kDvppErrorCreateDvppFail;
    }

    // step 2 The second call to the VPC interface
    // construct vpc parameters
    vpc_in_msg.format = dvpp_instance_para_.crop_or_resize_para.image_type;
    vpc_in_msg.cvdr_or_rdma = dvpp_instance_para_.crop_or_resize_para
            .cvdr_or_rdma;
    vpc_in_msg.bitwidth = dvpp_instance_para_.crop_or_resize_para.bit_width;
    vpc_in_msg.rank = dvpp_instance_para_.crop_or_resize_para.rank;
    vpc_in_msg.width = even_width;
    vpc_in_msg.high = even_high;

    vpc_in_msg.hmax = resize_out_param.hmax;
    vpc_in_msg.hmin = resize_out_param.hmin;
    vpc_in_msg.vmax = resize_out_param.vmax;
    vpc_in_msg.vmin = resize_out_param.vmin;
    vpc_in_msg.hinc = resize_out_param.hinc;
    vpc_in_msg.vinc = resize_out_param.vinc;

    // check scaling parameters, the scale factor must be [1/32, 4]
    ret = dvpp_utils.CheckIncreaseParam(vpc_in_msg.hinc, vpc_in_msg.vinc);

    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "resize hinc param must be [0.03125, 1) or (1, 4] and vinc "
                "param must be [0.03125, 1) or (1, 4]!, now hinc is %f, "
                "vinc " "is %f",
                vpc_in_msg.hinc, vpc_in_msg.vinc);
        DestroyDvppApi(pi_dvpp_api);
        return ret;
    }

    int in_buffer_size = 0;
    char *in_buffer = nullptr;

    // The flag whether input image is aligned
    bool is_input_align = dvpp_instance_para_.crop_or_resize_para.is_input_align;

    int width_stride = 0;

    // alloc input buffer
    ret = dvpp_utils.AllocBuffer(
            input_buf, input_size, is_input_align, vpc_in_msg.format,
            dvpp_instance_para_.crop_or_resize_para.src_resolution.width,
            dvpp_instance_para_.crop_or_resize_para.src_resolution.height,
            width_stride, in_buffer_size, &in_buffer);

    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR("Allocate vpc buffer failed!");
        DestroyDvppApi(pi_dvpp_api);
        return ret;
    }

    vpc_in_msg.stride = width_stride;

    vpc_in_msg.in_buffer = in_buffer;
    vpc_in_msg.in_buffer_size = in_buffer_size;

    shared_ptr<AutoBuffer> auto_out_buffer = make_shared<AutoBuffer>();
    vpc_in_msg.auto_out_buffer_1 = auto_out_buffer;

    dvpp_api_ctl_msg.in = (void *) (&vpc_in_msg);
    dvpp_api_ctl_msg.in_size = sizeof(vpc_in_msg);

    // call DVPP VPC interface
    if (DvppCtl(pi_dvpp_api, DVPP_CTL_VPC_PROC, &dvpp_api_ctl_msg)
            != kDvppOperationOk) {
        ASC_LOG_ERROR("call dvppctl process faild!");
        DestroyDvppApi(pi_dvpp_api);
        //free(in_buffer);
        munmap(in_buffer, (unsigned) (ALIGN_UP(in_buffer_size, MAP_2M)));
        return kDvppErrorDvppCtlFail;
    }

    int out_width = dvpp_instance_para_.crop_or_resize_para.dest_resolution
            .width;
    int out_high = dvpp_instance_para_.crop_or_resize_para.dest_resolution
            .height;

    // check image whether need to align
    int image_align = kImageNeedAlign;
    image_align = dvpp_utils.CheckImageNeedAlign(out_width, out_high);

    // If the output image need alignment, directly copy all memory.
    if ((image_align == kImageNotNeedAlign)
            || dvpp_instance_para_.crop_or_resize_para.is_output_align) {
        ret = memcpy_s(output_buf, output_size,
                       vpc_in_msg.auto_out_buffer_1->getBuffer(),
                       vpc_in_msg.auto_out_buffer_1->getBufferSize());
        CHECK_VPC_MEMCPY_S_RESULT(ret, in_buffer, pi_dvpp_api);
    } else {  // If image is not aligned, memory copy from line to line.
        char *vpc_out_buffer = vpc_in_msg.auto_out_buffer_1->getBuffer();

        // output buffer index, like array indexes,
        // start at 0
        int out_index = 0;

        // remain memory size in output buffer
        int remain_out_buffer_size = output_size;

        // 128 byte memory alignment in height direction of image
        int out_width_align = ALIGN_UP(out_width, kVpcWidthAlign);

        // 16 byte memory alignment in height direction of image
        int out_high_align = ALIGN_UP(out_high, kVpcHeightAlign);

        // y channel data copy
        for (int j = 0; j < out_high; ++j) {
            ret = memcpy_s(output_buf + (ptrdiff_t) out_index * out_width,
                           remain_out_buffer_size, vpc_out_buffer, out_width);
            CHECK_VPC_MEMCPY_S_RESULT(ret, in_buffer, pi_dvpp_api);

            // Point the pointer to next row of data
            vpc_out_buffer += out_width_align;
            out_index++;
            remain_out_buffer_size -= out_width;
        }

        // uv channel data copy
        vpc_out_buffer += (ptrdiff_t) (out_high_align - out_high)
                * out_width_align;

        for (int k = out_high; k < out_high + out_high / 2; ++k) {
            ret = memcpy_s(output_buf + (ptrdiff_t) out_index * out_width,
                           remain_out_buffer_size, vpc_out_buffer, out_width);
            CHECK_VPC_MEMCPY_S_RESULT(ret, in_buffer, pi_dvpp_api);

            // Point the pointer to next row of data
            vpc_out_buffer += out_width_align;
            out_index++;
            remain_out_buffer_size -= out_width;
        }
    }

    // free memory
    munmap(in_buffer, (unsigned) (ALIGN_UP(in_buffer_size, MAP_2M)));
    DestroyDvppApi(pi_dvpp_api);
    return ret;
}

int DvppProcess::DvppJpegChangeToYuv(const char *input_buf, int input_size,
                                     jpegd_yuv_data_info *output_data) {
    DvppUtils dvpp_utils;

    // check input param
    int ret = dvpp_utils.CheckJpegChangeToYuvParam(input_buf, input_size,
                                                   output_data);
    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "Jpeg change to yuv input param or output param can not be "
                "null!");
        return ret;
    }

    // construct jpeg decode parameter
    jpegd_raw_data_info jpegd_in_data;

    // Due to hardware constraints, the length of the input buf is 8 byte longer
    // than the actual bitstream.
    jpegd_in_data.jpeg_data_size = input_size + JPEGD_IN_BUFFER_SUFFIX;

    // Initial address 128-byte alignment, large-page apply for memory
    unsigned char* addr_orig = (unsigned char*) mmap(
            0, jpegd_in_data.jpeg_data_size + kJpegDAddressAlgin,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | API_MAP_VA32BIT, -1, 0);

    if (addr_orig == MAP_FAILED) {
        ASC_LOG_ERROR("Failed to malloc memory in dvpp(JpegD).");
        return kDvppErrorMallocFail;
    }

    // input data need 128-byte alignment
    jpegd_in_data.jpeg_data = (unsigned char*) ALIGN_UP((uint64_t )addr_orig,
                                                        kJpegDAddressAlgin);

    ret = memcpy_s(jpegd_in_data.jpeg_data, jpegd_in_data.jpeg_data_size,
                   input_buf, input_size);
    CHECK_MEMCPY_RESULT(ret, nullptr);

    jpegd_in_data.IsYUV420Need = dvpp_instance_para_.jpegd_para
            .is_convert_yuv420;

    // construct dvpp parameters used in jpegD interface
    dvppapi_ctl_msg dvpp_api_ctl_msg;
    dvpp_api_ctl_msg.in = (void *) &jpegd_in_data;
    dvpp_api_ctl_msg.in_size = sizeof(jpegd_in_data);
    dvpp_api_ctl_msg.out = (void *) output_data;
    dvpp_api_ctl_msg.out_size = sizeof(jpegd_yuv_data_info);

    // create dvppApi handle
    IDVPPAPI *dvpp_api = nullptr;
    ret = CreateDvppApi(dvpp_api);

    if (dvpp_api != nullptr && ret == kDvppReturnOk) {
        // call DVPP  JPEGD to process
        if (DvppCtl(dvpp_api, DVPP_CTL_JPEGD_PROC, &dvpp_api_ctl_msg)
                != kDvppOperationOk) {
            ASC_LOG_ERROR("call dvppctl process failed\n");
            ret = kDvppErrorDvppCtlFail;
        }

        // free dvppapi handle
        DestroyDvppApi(dvpp_api);
    } else {
        ASC_LOG_ERROR("can not create dvpp api\n");
        ret = kDvppErrorCreateDvppFail;
    }

    // release buffer
    if (addr_orig != MAP_FAILED) {
        munmap(addr_orig,
               (unsigned) (ALIGN_UP(
                       jpegd_in_data.jpeg_data_size + kJpegDAddressAlgin,
                       MAP_2M)));
    }
    return ret;
}

int DvppProcess::DvppBasicVpc(const uint8_t *input_buf, int32_t input_size,
                              int32_t output_size, uint8_t *output_buf) {
    DvppUtils dvpp_utils;

    // check input and output param
    int ret = dvpp_utils.CheckBasicVpcParam(input_buf, input_size, output_size,
                                            output_buf);

    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "input_buf and output_buf can not be null, input_size and "
                "output_size can not less than 0, now input_size is %d and "
                "output_size is %d !",
                input_size, output_size);
        return ret;
    }

    // check image format params
    VpcInputFormat input_format = dvpp_instance_para_.basic_vpc_para
            .input_image_type;
    VpcOutputFormat output_format = dvpp_instance_para_.basic_vpc_para
            .output_image_type;
    ret = dvpp_utils.CheckBasicVpcImageFormat(input_format, output_format);

    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "Input image format or output image format is out of range, "
                "input format is %d, output format is %d",
                input_format, output_format);
        return ret;
    }

    // check crop params
    uint32_t left_offset = dvpp_instance_para_.basic_vpc_para.crop_left;
    uint32_t up_offset = dvpp_instance_para_.basic_vpc_para.crop_up;
    uint32_t right_offset = dvpp_instance_para_.basic_vpc_para.crop_right;
    uint32_t down_offset = dvpp_instance_para_.basic_vpc_para.crop_down;

    ret = dvpp_utils.CheckBasicVpcCropParam(left_offset, up_offset,
                                            right_offset, down_offset);
    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "The left_offset and up_offset params must be even, The "
                "right_offset and down_offset params must be odd, left_offset "
                "is %d, up_offset is %d, right_offset is %d, down_offset is %d",
                left_offset, up_offset, right_offset, down_offset);

        return ret;
    }

    // check output image params
    int output_width = dvpp_instance_para_.basic_vpc_para.dest_resolution.width;
    int output_height = dvpp_instance_para_.basic_vpc_para.dest_resolution
            .height;

    ret = dvpp_utils.CheckBasicVpcOutputParam(output_width, output_height);
    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR(
                "The width and height of the output image must be even, output "
                "width is %d, output height is %d",
                output_width, output_height);

        return ret;
    }

    int input_width = dvpp_instance_para_.basic_vpc_para.src_resolution.width;
    int input_height = dvpp_instance_para_.basic_vpc_para.src_resolution.height;
    int height_stride = ALIGN_UP((input_height >> 1) << 1, kVpcHeightAlign);
    int width_stride = 0;

    int in_buffer_size = 0;
    uint8_t *in_buffer = nullptr;

    // The flag whether input image is aligned
    bool is_input_align = dvpp_instance_para_.basic_vpc_para.is_input_align;

    // alloc input buffer
    ret = dvpp_utils.AllocBasicVpcBuffer(input_buf, input_size, is_input_align,
                                         input_format, input_width,
                                         input_height, width_stride,
                                         in_buffer_size, &in_buffer);

    if (ret != kDvppOperationOk) {
        ASC_LOG_ERROR("Allocate basic vpc buffer failed!");
        return ret;
    }

    // constructing input image configuration
    shared_ptr<VpcUserImageConfigure> image_configure(
            new VpcUserImageConfigure);

    image_configure->bareDataAddr = in_buffer;
    image_configure->bareDataBufferSize = in_buffer_size;
    image_configure->isCompressData = false;
    image_configure->widthStride = width_stride;
    image_configure->heightStride = height_stride;
    image_configure->inputFormat = input_format;
    image_configure->outputFormat = output_format;
    image_configure->yuvSumEnable = false;
    image_configure->cmdListBufferAddr = nullptr;
    image_configure->cmdListBufferSize = 0;

    shared_ptr<VpcUserRoiConfigure> roi_configure(new VpcUserRoiConfigure);
    roi_configure->next = nullptr;

    // constructing input roi configuration
    VpcUserRoiInputConfigure *input_configure = &roi_configure->inputConfigure;
    input_configure->cropArea.leftOffset = dvpp_instance_para_.basic_vpc_para
            .crop_left;
    input_configure->cropArea.rightOffset = dvpp_instance_para_.basic_vpc_para
            .crop_right;
    input_configure->cropArea.upOffset = dvpp_instance_para_.basic_vpc_para
            .crop_up;
    input_configure->cropArea.downOffset = dvpp_instance_para_.basic_vpc_para
            .crop_down;

    int aligned_output_width = ALIGN_UP(output_width, kVpcWidthAlign);
    int aligned_output_height = ALIGN_UP(output_height, kVpcHeightAlign);

    int vpc_output_size = aligned_output_width * aligned_output_height
            * DVPP_YUV420SP_SIZE_MOLECULE /
    DVPP_YUV420SP_SIZE_DENOMINATOR;

    uint8_t *out_buffer = (uint8_t *) mmap(
            0, ALIGN_UP(vpc_output_size, MAP_2M),
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | API_MAP_VA32BIT, -1, 0);

    if (out_buffer == MAP_FAILED) {
        ASC_LOG_INFO(
                "Failed to malloc memory in dvpp(new vpc), start to try 4K "
                "memory");
        out_buffer = (uint8_t *) mmap(
                0, ALIGN_UP(vpc_output_size, MAP_2M),
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | API_MAP_VA32BIT, -1, 0);
        if (out_buffer == MAP_FAILED) {
            ASC_LOG_ERROR("4K memory malloc still fail.");
            return kDvppErrorMallocFail;
        }
    }

    // constructing output roi configuration
    VpcUserRoiOutputConfigure *output_configure = &roi_configure
            ->outputConfigure;
    output_configure->addr = out_buffer;
    output_configure->bufferSize = vpc_output_size;
    output_configure->widthStride = aligned_output_width;
    output_configure->heightStride = aligned_output_height;
    output_configure->outputArea.leftOffset = 0;
    output_configure->outputArea.rightOffset = output_width - 1;
    output_configure->outputArea.upOffset = 0;
    output_configure->outputArea.downOffset = output_height - 1;

    image_configure->roiConfigure = roi_configure.get();

    dvppapi_ctl_msg dvpp_api_ctl_msg;
    dvpp_api_ctl_msg.in = static_cast<void *>(image_configure.get());

    dvpp_api_ctl_msg.in_size = sizeof(VpcUserImageConfigure);

    // create dvpp api
    IDVPPAPI *pi_dvpp_api = nullptr;
    ret = CreateDvppApi(pi_dvpp_api);

    // call DVPP VPC interface
    if (pi_dvpp_api != nullptr && ret == kDvppReturnOk) {
        if (DvppCtl(pi_dvpp_api, DVPP_CTL_VPC_PROC, &dvpp_api_ctl_msg)
                != kDvppOperationOk) {
            ASC_LOG_ERROR("call dvppctl process faild!");
            DestroyDvppApi(pi_dvpp_api);
            //free memory
            munmap(in_buffer, (unsigned) (ALIGN_UP(in_buffer_size, MAP_2M)));
            munmap(out_buffer, (unsigned) (ALIGN_UP(vpc_output_size, MAP_2M)));
            return kDvppErrorDvppCtlFail;
        }
    } else {  // create dvpp api fail, directly return
        ASC_LOG_ERROR("pi_dvpp_api is null!");
        DestroyDvppApi(pi_dvpp_api);
        //free memory
        munmap(in_buffer, (unsigned) (ALIGN_UP(in_buffer_size, MAP_2M)));
        munmap(out_buffer, (unsigned) (ALIGN_UP(vpc_output_size, MAP_2M)));

        return kDvppErrorCreateDvppFail;
    }

    munmap(in_buffer, (unsigned) (ALIGN_UP(in_buffer_size, MAP_2M)));

    // check image whether need to align
    int image_align = kImageNeedAlign;
    image_align = dvpp_utils.CheckImageNeedAlign(output_width, output_height);

    // If the output image need alignment, directly copy all memory.
    if ((image_align == kImageNotNeedAlign)
            || dvpp_instance_para_.basic_vpc_para.is_output_align) {
        ret = memcpy_s(output_buf, output_size, out_buffer, vpc_output_size);
        CHECK_VPC_MEMCPY_S_RESULT(ret, out_buffer, pi_dvpp_api);
    } else {  // If image is not aligned, memory copy from line to line.
        uint8_t *vpc_out_buffer = out_buffer;

        // output buffer index, like array indexes,
        // start at 0
        int out_index = 0;

        // remain memory size in output buffer
        int remain_out_buffer_size = output_size;

        // y channel data copy
        for (int j = 0; j < output_height; ++j) {
            ret = memcpy_s(output_buf + (ptrdiff_t) out_index * output_width,
                           remain_out_buffer_size, vpc_out_buffer,
                           output_width);
            CHECK_VPC_MEMCPY_S_RESULT(ret, out_buffer, pi_dvpp_api);

            // Point the pointer to next row of data
            vpc_out_buffer += aligned_output_width;
            out_index++;
            remain_out_buffer_size -= output_width;
        }

        // uv channel data copy
        vpc_out_buffer += (ptrdiff_t) (aligned_output_height - output_height)
                * aligned_output_width;

        for (int k = output_height; k < output_height + output_height / 2;
                ++k) {
            ret = memcpy_s(output_buf + (ptrdiff_t) out_index * output_width,
                           remain_out_buffer_size, vpc_out_buffer,
                           output_width);
            CHECK_VPC_MEMCPY_S_RESULT(ret, out_buffer, pi_dvpp_api);

            // Point the pointer to next row of data
            vpc_out_buffer += aligned_output_width;
            out_index++;
            remain_out_buffer_size -= output_width;
        }
    }

    DestroyDvppApi(pi_dvpp_api);
    munmap(out_buffer, (unsigned) (ALIGN_UP(vpc_output_size, MAP_2M)));
    return ret;
}
}
}
