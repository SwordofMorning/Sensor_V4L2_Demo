#pragma once

/**
 * @file v4l2_dvp.h
 * @author xjt.include@gmai.com
 * @brief IR camera DVP interface program.
 * @version 0.1
 * @date 2023-04-11
 * 
 * @copyright Copyright (c) 2023
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <assert.h>  
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <errno.h>  
#include <stdint.h>
#include <stdbool.h>
#include <omp.h>

/* ===== Define Begin ===== */
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define V4L2_IR_DVP_DEVICE_NAME "/dev/video0"
#define V4L2_IR_DVP_PIX_FMT V4L2_PIX_FMT_GREY
#define V4L2_IR_DVP_REQ_COUNT 5

extern const int v4l2_ir_dvp_capture_width;
extern const int v4l2_ir_dvp_capture_height;
extern int v4l2_ir_dvp_fd;
extern int v4l2_ir_dvp_nplanes;
extern int v4l2_ir_dvp_buffer_global_index;
extern int v4l2_ir_dvp_buffer_global_length;

/* ===== Define End ===== */

#ifdef __cplusplus
extern "C" {
#endif

struct v4l2_ir_dvp_buffer {
    void *start;
    size_t length;
};

extern struct v4l2_ir_dvp_buffer *v4l2_ir_dvp_buffer_global;

/**
 * @brief Start DVP camera streaming, restore data in v4l2_ir_dvp_buffer_global.
 * 
 * @return nothing now.
 */
int DVP_Streaming();

#ifdef __cplusplus
}
#endif