#include <stdio.h>
#include "librtmp_sendAAC.h"
#include "rtmp_net.h"


FILE *fp_send1;

//读文件的回调函数
//we use this callback function to read data from buffer
int read_buffer1(unsigned char *buf, int buf_size ){
	if(!feof(fp_send1)){
		int true_size=fread(buf,1,buf_size,fp_send1);
		return true_size;
	}else{
		return -1;
	}
}

//#include<windows.h>

int main(int argc, char* argv[])
{	
	//初始化并连接到服务器
	Net_Init("rtmp://192.168.0.5:1935/live");
	//发送
	RTMPAAC_Send();
	//断开连接并释放相关资源
	Net_Close();

	return 0;
}

