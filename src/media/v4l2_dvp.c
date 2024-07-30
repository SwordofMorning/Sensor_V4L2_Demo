#include "v4l2_dvp.h"

struct v4l2_ir_dvp_buffer *v4l2_ir_dvp_buffer_global;

const int v4l2_ir_dvp_capture_width = 1280;
const int v4l2_ir_dvp_capture_height = 513;
int v4l2_ir_dvp_fd = 0;
int v4l2_ir_dvp_nplanes = 0;
int v4l2_ir_dvp_buffer_global_index= 0;
int v4l2_ir_dvp_buffer_global_length = 0;

static void DVP_Error(const char* error_str)
{
	printf("%s\n", error_str);
	exit(EXIT_FAILURE);
}

static int DVP_Open_Device()
{
	v4l2_ir_dvp_fd = open(V4L2_IR_DVP_DEVICE_NAME, O_RDWR /*| O_NONBLOCK*/, 0);
	if (v4l2_ir_dvp_fd == -1) DVP_Error("DVP_Open_Device() open camera fail.");
	return v4l2_ir_dvp_fd;
}

static int DVP_MMAP()
{
	/* Step 1 : Request Buffer */
	struct v4l2_requestbuffers reqbuf;  
	CLEAR(reqbuf);  

	reqbuf.count = V4L2_IR_DVP_REQ_COUNT;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	reqbuf.memory = V4L2_MEMORY_MMAP;

	if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_REQBUFS, &reqbuf))
	{
		if (errno == EINVAL) DVP_Error("DVP_MMAP() not support mmap.");
		else DVP_Error("DVP_MMAP() mmap other error.");
	}

	/* Step 2 : mmap Buffer */
	v4l2_ir_dvp_buffer_global = calloc(reqbuf.count, sizeof(*v4l2_ir_dvp_buffer_global));
	if (!v4l2_ir_dvp_buffer_global) DVP_Error("DVP_MMAP() calloc error.");

	for (int i = 0; i < reqbuf.count; ++i)
	{
		struct v4l2_buffer buff;
		CLEAR(buff);

		buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buff.memory = V4L2_MEMORY_MMAP;
		buff.index = i;

		struct v4l2_plane planes[VIDEO_MAX_PLANES];
		memset(planes, 0, VIDEO_MAX_PLANES * sizeof(struct v4l2_plane));
		/**
		 * @note when type=V4L2_BUF_TYPE_VIDEO_CAPTURE, length means frame size
		 * @note when type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, length means how many planes 
		 */
		buff.length = v4l2_ir_dvp_nplanes;
		buff.m.planes = planes;

		if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_QUERYBUF, &buff)) 
			DVP_Error("DVP_MMAP() VIDIOC_QUERYBUF fail.");

		/**
		 * @note usr_buffer[0].start = frame addr, but not planes[0].m.mem_offset
		 * 
		 * @note user_buffer[0].start = 
		 * 		planes[0].m.mem_offset + 
		 * 		planes[1].m.mem_offset + ... 
		 * 		planes[nplanes-1].m.mem_offset
		 * 
		 * @note in this case, nplanes == 1, so frame buffer have only one plane
		 */
		v4l2_ir_dvp_buffer_global[i].start = 
			(uint8_t*)mmap(NULL, buff.m.planes[0].length,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			v4l2_ir_dvp_fd,
			buff.m.planes[0].m.mem_offset);
		v4l2_ir_dvp_buffer_global[i].length = buff.m.planes[0].length;
		v4l2_ir_dvp_buffer_global_length = v4l2_ir_dvp_buffer_global[i].length;
		
		printf("mmap buffer index[%d], length = %d, phyaddr = %p, viraddr = %p\n",
			buff.index, v4l2_ir_dvp_buffer_global[i].length,
			buff.m.planes[0].m.mem_offset, v4l2_ir_dvp_buffer_global[i].start);
		
		if (MAP_FAILED == v4l2_ir_dvp_buffer_global[i].start) DVP_Error("DVP_MMAP() mmap fail.");
		if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_QBUF, &buff)) DVP_Error("DVP_MMAP() VIDIOC_QBUF fail.");
	}
	return 0;
}

static int DVP_Init_Device()
{
	DVP_Open_Device();

	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;  
	struct v4l2_crop crop;  
	struct v4l2_format fmt;  
	unsigned int min; 

	/* Step 1 : Check Capability */
	if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_QUERYCAP, &cap))
	{
		if (errno = EINVAL)
			DVP_Error("DVP_Init_Device() non V4l2 device.");
		else
			DVP_Error("DVP_Init_Device() VIDIOC_QUERYCAP.");
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) 
		DVP_Error("DVP_Init_Device() non capture device.");
	if (!(cap.capabilities & V4L2_CAP_STREAMING))
		DVP_Error("DVP_Init_Device() not support streaming.");

	/* Step 2 : Set Cropcap */
	// CLEAR(cropcap);

	// cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; 

	/**
	 * @brief if device support cropcap's type, then set crop
	 * @note it's seems cannot do this in C, but use v4l2-ctl command:
	 * @note v4l2-ctl -d /dev/video0 --set-crop top=0,left=0,width=768,height=288
	 */
	// if (0 == ioctl(v4l2_ir_dvp_fd, VIDIOC_CROPCAP, &cropcap))
	// {
	// 	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	// 	crop.c.top = 0;
	// 	crop.c.left = 0;
	// 	crop.c.width = v4l2_ir_dvp_capture_width;
	// 	crop.c.height = v4l2_ir_dvp_capture_height;

	// 	if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_S_CROP, &crop))
	// 	{
	// 		if (errno == EINVAL) DVP_Error("DVP_Init_Device() crop not support.");
	// 		else DVP_Error("DVP_Init_Device() crop other error.");
	// 	}
	// }
	// else
	// {
	// 	if (errno == EINVAL) DVP_Error("DVP_Init_Device() the struct v4l2_cropcap type is invalid.");
	// 	else DVP_Error("DVP_Init_Device() cropping is not supported for this input or output.");
	// }
	system("v4l2-ctl -d /dev/video0 --set-crop top=0,left=0,width=1280,height=513");

	/* Step 3 : Set FMT */
	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width = v4l2_ir_dvp_capture_width;
	fmt.fmt.pix_mp.height = v4l2_ir_dvp_capture_height;
	fmt.fmt.pix_mp.pixelformat = V4L2_IR_DVP_PIX_FMT;

	if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_S_FMT, &fmt))
		DVP_Error("DVP_Init_Device() set fmt fail.");

	v4l2_ir_dvp_nplanes = fmt.fmt.pix_mp.num_planes;
	printf("v4l2_ir_dvp_nplanes = %d\n", v4l2_ir_dvp_nplanes);

	/* Step 4 : Init MMAP */
	return DVP_MMAP();
}

static int DVP_Start_Capture()
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_STREAMON, &type))
		DVP_Error("DVP_Start_Capture() VIDIOC_STREAMON.");

	return 0;
}

static int DVP_Save(FILE* fp)
{
	// FILE* fp = fopen("out.yuv", "a");

	// fwrite(v4l2_ir_dvp_buffer_global[v4l2_ir_dvp_buffer_global_index].start, 
	// 	v4l2_ir_dvp_buffer_global_length, sizeof(uint8_t), fp);

    fwrite(v4l2_ir_dvp_buffer_global[v4l2_ir_dvp_buffer_global_index].start, 
		640*512*2, sizeof(uint8_t), fp);

	// fclose(fp);

	return 0;
}

static int DVP_Capture()
{
    FILE* fp = fopen("out.yuv", "a");

    int frames = 100;
    int captured_frames = 0;
    
    // 添加计时器
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    struct v4l2_buffer buff;
    CLEAR(buff);
    struct v4l2_plane planes[VIDEO_MAX_PLANES];  
    memset(planes, 0, VIDEO_MAX_PLANES * sizeof(struct v4l2_plane));

    buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buff.memory = V4L2_MEMORY_MMAP;
    buff.length = v4l2_ir_dvp_nplanes;
    buff.m.planes = planes;

    for (int i = 0; i < frames; ++i)
    {
        if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_DQBUF, &buff))
            if (errno == EAGAIN) DVP_Error("DVP_Capture() VIDIOC_DQBUF EAGAIN.");
            else if (errno == EINVAL) DVP_Error("DVP_Capture() VIDIOC_DQBUF EINVAL.");
            else if (errno == EIO) DVP_Error("DVP_Capture() VIDIOC_DQBUF EIO.");
            else if (errno == EPIPE) DVP_Error("DVP_Capture() VIDIOC_DQBUF EPIPE.");

        // process data
        v4l2_ir_dvp_buffer_global_index = buff.index;
        DVP_Save(fp);
        
        // 记录采集的帧数
        captured_frames++;

#if Print_Debug_Info
    printf("DVP_TEMP: %x %x\n", 
        v4l2_ir_dvp_buffer_global[v4l2_ir_dvp_buffer_global_index].start,
        v4l2_ir_dvp_buffer_global[v4l2_ir_dvp_buffer_global_index].start + 1);
#endif

        if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_QBUF, &buff))
            DVP_Error("DVP_Capture() fail to VIDIOC_QBUF.");
    }

    // 记录结束时间
    gettimeofday(&end_time, NULL);

    // 计算采集的总时间（秒）
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                          (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    // 计算平均帧率
    double fps = captured_frames / elapsed_time;

    printf("Captured %d frames in %.2f seconds. Average FPS: %.2f\n", 
           captured_frames, elapsed_time, fps);

    fclose(fp);
    return 0;
}

static int DVP_Stop_Capture()
{
	enum v4l2_buf_type type;                                                
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	if (-1 == ioctl(v4l2_ir_dvp_fd, VIDIOC_STREAMOFF, &type))
		DVP_Error("DVP_Start_Capture() VIDIOC_STREAMOFF.");

	return 0;
}

static int DVP_Close_Device()
{
	if (-1 == close(v4l2_ir_dvp_fd))
		DVP_Error("DVP_Close_Device() fai.");
	return 0;
}

static int DVP_MUNMAP()
{
	for (int i = 0; i < V4L2_IR_DVP_REQ_COUNT; ++i)
	{
		if (-1 == munmap(v4l2_ir_dvp_buffer_global[i].start, v4l2_ir_dvp_buffer_global->length))
			DVP_Error("DVP_MUNMAP() munmap fai.");
	}
}

static int DVP_Exit_Device()
{
	DVP_MUNMAP();
	DVP_Close_Device();
}

int DVP_Streaming()
{
	DVP_Init_Device();
	DVP_Start_Capture();

	DVP_Capture();

	DVP_Stop_Capture();
	DVP_Exit_Device();
	return 0;
}