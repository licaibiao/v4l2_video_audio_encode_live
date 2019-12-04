#ifndef V4L2_DEVICE_H
#define V4L2_DEVICE_H
extern "C"{
	
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <stdio.h>  
#include <sys/ioctl.h>  
#include <stdlib.h>  
#include <linux/types.h>  
#include <linux/videodev2.h>  
#include <malloc.h>  
#include <math.h>  
#include <string.h>  
#include <sys/mman.h>  
#include <errno.h>  
#include <assert.h>  
  
#define FILE_VIDEO  "/dev/video0"  

int open_camera(void);
int init_camera(int fd, int width, int height);
int start_capture(int fd);
int read_frame(int fd, unsigned char *outbuf, int *len);  
void stop_capture(int fd);
void close_camera_device(int fd);
 
}

#endif

