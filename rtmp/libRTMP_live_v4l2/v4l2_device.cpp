/*============================================================================= 
#     FileName: v4l2_device.c 
#         Desc: this program aim to get image from USB camera, 
#               used the V4L2 interface. 
#       Author: Licaibiao 
#      Version:  
#   LastChange: 2017-5-9  
=============================================================================*/  
#include "v4l2_device.h"  

typedef struct{  
    void *start;  
    int length;  
}BUFTYPE;  
  
BUFTYPE *usr_buf;  
static unsigned int n_buffer = 0;  
  
/*set video capture ways(mmap)*/  
int init_mmap(int fd){  
    /*to request frame cache, contain requested counts*/  
    struct v4l2_requestbuffers reqbufs;  
  
    memset(&reqbufs, 0, sizeof(reqbufs));  
    reqbufs.count = 4;                              /*the number of buffer*/  
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;      
    reqbufs.memory = V4L2_MEMORY_MMAP;                
  
    if(-1 == ioctl(fd,VIDIOC_REQBUFS,&reqbufs))    {  
        perror("Fail to ioctl 'VIDIOC_REQBUFS'");  
        exit(EXIT_FAILURE);  
    }  
      
    n_buffer = reqbufs.count;  
    printf("n_buffer = %d\n", n_buffer);  
    usr_buf = (BUFTYPE *)calloc(reqbufs.count, sizeof(BUFTYPE));  
    if(usr_buf == NULL){  
        printf("Out of memory\n");  
        exit(-1);  
    }  
  
    /*map kernel cache to user process*/  
    for(n_buffer = 0; n_buffer < reqbufs.count; ++n_buffer){  
        //stand for a frame  
        struct v4l2_buffer buf;  
        memset(&buf, 0, sizeof(buf));  
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        buf.memory = V4L2_MEMORY_MMAP;  
        buf.index = n_buffer;  
          
        /*check the information of the kernel cache requested*/  
        if(-1 == ioctl(fd,VIDIOC_QUERYBUF,&buf)){  
            perror("Fail to ioctl : VIDIOC_QUERYBUF");  
            exit(EXIT_FAILURE);  
        }  
  
        usr_buf[n_buffer].length = buf.length;  
        usr_buf[n_buffer].start = (char *)mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED, fd,buf.m.offset);  
  
        if(MAP_FAILED == usr_buf[n_buffer].start){  
            perror("Fail to mmap");  
            exit(EXIT_FAILURE);  
        }  
    }
	return 0;  
}  
  
int open_camera(void)  
{  
    int fd;  
    struct v4l2_input inp;  
  
    fd = open(FILE_VIDEO, O_RDWR | O_NONBLOCK,0);  
    if(fd < 0){     
        fprintf(stderr, "%s open err \n", FILE_VIDEO);  
        exit(EXIT_FAILURE);  
    };  
  
    inp.index = 0;  
    if (-1 == ioctl (fd, VIDIOC_S_INPUT, &inp)){  
        fprintf(stderr, "VIDIOC_S_INPUT \n");  
    }  
  
    return fd;  
}  
  
int init_camera(int fd,int width, int height)  
{  
    struct v4l2_capability  cap;    /* decive fuction, such as video input */  
    struct v4l2_format      tv_fmt; /* frame format */    
    struct v4l2_fmtdesc     fmtdesc;    /* detail control value */  
    //struct v4l2_control     ctrl;  
    int ret;  
      
            /*show all the support format*/  
    memset(&fmtdesc, 0, sizeof(fmtdesc));  
    fmtdesc.index = 0 ;                 /* the number to check */  
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;  
  
    /* check video decive driver capability */  
    ret=ioctl(fd, VIDIOC_QUERYCAP, &cap);  
    if(ret < 0){  
        fprintf(stderr, "fail to ioctl VIDEO_QUERYCAP \n");  
        exit(EXIT_FAILURE);  
    }  
      
    /*judge wherher or not to be a video-get device*/  
    if(!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE)){  
        fprintf(stderr, "The Current device is not a video capture device \n");  
        exit(EXIT_FAILURE);  
    }  
  
    /*judge whether or not to supply the form of video stream*/  
    if(!(cap.capabilities & V4L2_CAP_STREAMING)){  
        printf("The Current device does not support streaming i/o\n");  
        exit(EXIT_FAILURE);  
    }  
      
    printf("\ncamera driver name is : %s\n",cap.driver);  
    printf("camera device name is : %s\n",cap.card);  
    printf("camera bus information: %s\n",cap.bus_info);  
  
    /*display the format device support*/  
    printf("\n");  
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1){     
        printf("support device %d.%s\n",fmtdesc.index+1,fmtdesc.description);  
        fmtdesc.index++;  
    }  
    printf("\n");  
  
    /*set the form of camera capture data*/  
    tv_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;      /*v4l2_buf_typea,camera must use V4L2_BUF_TYPE_VIDEO_CAPTURE*/  
    tv_fmt.fmt.pix.width = width;  
    tv_fmt.fmt.pix.height = height;  
    //tv_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;   /*V4L2_PIX_FMT_YYUV*/  
    tv_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;   /*V4L2_PIX_FMT_YYUV*/  
    tv_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;         /*V4L2_FIELD_NONE*/  
    if (ioctl(fd, VIDIOC_S_FMT, &tv_fmt)< 0){  
        fprintf(stderr,"VIDIOC_S_FMT set err\n");  
        exit(-1);  
        close(fd);  
    }  
  
    init_mmap(fd);
	
	return 0;  
}  
  
int start_capture(int fd)  
{  
    unsigned int i;  
    enum v4l2_buf_type type;  
      
    /*place the kernel cache to a queue*/  
    for(i = 0; i < n_buffer; i++){  
        struct v4l2_buffer buf;  
        memset(&buf, 0, sizeof(buf));  
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        buf.memory = V4L2_MEMORY_MMAP;  
        buf.index = i;  
  
        if(-1 == ioctl(fd, VIDIOC_QBUF, &buf)){  
            perror("Fail to ioctl 'VIDIOC_QBUF'");  
            exit(EXIT_FAILURE);  
        }  
    }  
  
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    if(-1 == ioctl(fd, VIDIOC_STREAMON, &type)){  
        printf("i=%d.\n", i);  
        perror("VIDIOC_STREAMON");  
        close(fd);  
        exit(EXIT_FAILURE);  
    }  
  
    return 0;  
}  
 
int read_frame(int fd, unsigned char *outbuf, int *len)  
{  
    struct v4l2_buffer buf;  
    //unsigned int i; 

	fd_set fds;  
    struct timeval tv;  
    int r;  
  
    FD_ZERO(&fds);  
    FD_SET(fd,&fds);  
  
    /*Timeout*/  
    tv.tv_sec = 2;  
    tv.tv_usec = 0;  
    r = select(fd + 1,&fds,NULL,NULL,&tv);  
      
    if(-1 == r){  
        if(EINTR == errno){
		printf("select received SIGINT \n");
		return 0;
        //perror("Fail to select");  
        //exit(EXIT_FAILURE);  
		}  
    }  
    if(0 == r){  
        fprintf(stderr,"select Timeout\n");  
        exit(-1);  
    }  
	
    memset(&buf, 0, sizeof(buf));  
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    buf.memory = V4L2_MEMORY_MMAP;  

    if(-1 == ioctl(fd, VIDIOC_DQBUF,&buf)){  
        perror("Fail to ioctl 'VIDIOC_DQBUF'");  
        exit(EXIT_FAILURE);  
    }  
    assert(buf.index < n_buffer);
	//unsigned char outbuf[1024*1024];  
	//process_image(usr_buf[buf.index].start, usr_buf[buf.index].length); 
    memcpy(outbuf, usr_buf[buf.index].start, usr_buf[buf.index].length);
	*len = usr_buf[buf.index].length; 
    //memmove(outbuf, usr_buf[buf.index].start, usr_buf[buf.index].length);
	
    //FILE* fp = fopen("test.yuyv","w");
	//fwrite( usr_buf[buf.index].start,1,usr_buf[buf.index].length,fp);
	//fclose(fp);
    if(-1 == ioctl(fd, VIDIOC_QBUF,&buf)){  
        perror("Fail to ioctl 'VIDIOC_QBUF'");  
        exit(EXIT_FAILURE);  
    }  
    return 1;  
}  
  
void stop_capture(int fd)  
{  
    enum v4l2_buf_type type;  
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    if(-1 == ioctl(fd,VIDIOC_STREAMOFF,&type)){  
        perror("Fail to ioctl 'VIDIOC_STREAMOFF'");  
        exit(EXIT_FAILURE);  
    }  
}  
  
void close_camera_device(int fd)  
{  
    unsigned int i;  
    for(i = 0;i < n_buffer; i++){  
        if(-1 == munmap(usr_buf[i].start,usr_buf[i].length)){  
            exit(-1);  
        }  
    }  
    free(usr_buf);  
    if(-1 == close(fd)){  
        perror("Fail to close fd");  
        exit(EXIT_FAILURE);  
    }  
}  
  


