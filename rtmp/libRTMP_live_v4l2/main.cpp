/*=============================================================================  
 *     FileName: main.cpp
 *         Desc:  
 *       Author: licaibiao  
 *   LastChange: 2017-05-8   
 * =============================================================================*/  
#include "x264_encoder.h"
#include "v4l2_device.h"
#include "librtmp/log.h"
#include <signal.h>
#include <error.h>

int runflag=0;
static void sig_user(int signo){
    if(signo==SIGINT){
    	runflag=0;
        printf("received SIGINT\n");
  }
}

void rtmp_push_v4l2(){
	char url[]="rtmp://192.168.0.5:1935/live";
	int fps		= 30;
	int rate	= 333;
	int width	= 640;
	int height	= 480;
	int outSize	= 1024;

	int index=0;	
	unsigned int tick = 0;
	unsigned int tick_gap = 1000/fps;
	uint32_t now=0;
	uint32_t last_update=0;

	int fd;
	int len = 0;
	uint8_t *cam_buf;
	uint32_t pixelformat;

	cam_buf = (uint8_t*)malloc(1024*1024*3);
	memset(cam_buf, 0, 1024*1024*3);
	
	pixelformat = V4L2_PIX_FMT_YUYV;
	
    if(signal(SIGINT,sig_user)==SIG_ERR){
		perror("catch SIGINT err");
	}

	fd =open_camera();
	init_camera(fd, width, height);
	start_capture(fd);
		
	//RTMP_CreatePublish(url,outSize,1,RTMP_LOGINFO);
	RTMP_CreatePublish(url,outSize,1,RTMP_LOGERROR);
	printf("connected \n");
	RTMP_InitVideoParams(width,height,fps, rate, pixelformat,false);
	printf("inited \n");
	runflag=1;
	//runflag=3;
		
	while(runflag){
		if(index!=0){
			RTMP_SendScreenCapture(cam_buf,height,tick, pixelformat, width, height);
		//	printf("send frame index -- %d\n",index);
		}
		last_update=RTMP_GetTime();

		read_frame(fd, cam_buf,&len);

		tick +=tick_gap;
		now=RTMP_GetTime();
		
		//usleep((tick_gap-now+last_update)*1000);
		usleep(1000);
		index++;
	}

	free(cam_buf);
	stop_capture(fd);
	close_camera_device(fd);
	RTMP_DeletePublish();
}

int main(){
	rtmp_push_v4l2();
	return 0;
}


