#ifndef LIB_RTMP_SEND_H264_H
#define LIB_RTMP_SEND_H264_H
extern "C" {

#include "librtmp/rtmp.h"
#include "librtmp/rtmp_sys.h"
#include "librtmp/amf.h"


/**
* _NaluUnit
* 内部结构体。该结构体主要用于存储和传递Nal单元的类型、大小和数据
*/
typedef struct _NaluUnit
{
	int type;
	int size;
	unsigned char *data;
}NaluUnit;

/**
* _RTMPMetadata
* 内部结构体。该结构体主要用于存储和传递元数据信息
*/
typedef struct _RTMPMetadata
{
	// video, must be h264 type
	unsigned int    nWidth;
	unsigned int    nHeight;
	unsigned int    nFrameRate;
	unsigned int    nSpsLen;
	unsigned char   *Sps;
	unsigned int    nPpsLen;
	unsigned char   *Pps;
} RTMPMetadata, *LPRTMPMetadata;

enum
{
	VIDEO_CODECID_H264 = 7,
};



/**
 * 初始化并连接到服务器
 *
 * @param url 服务器上对应webapp的地址
 * @isOpenPrintLog 是否打印日志
 * @logType 日志类型
 * @成功则返回1 , 失败则返回0
 */
int RTMP264_Connect(const char* url, RTMP** ppRtmp, int isOpenPrintLog, int logType);


/**
 * 断开连接，释放相关的资源。
 *
 */
void RTMP264_Close();

void InitSpsPps(unsigned char *pps, int pps_len, unsigned char * sps, int sps_len, int width, int height, int fps);


/**
* 发送H264数据帧
*
* @param data 存储数据帧内容
* @param size 数据帧的大小
* @param bIsKeyFrame 记录该帧是否为关键帧
* @param nTimeStamp 当前帧的时间戳
*
* @成功则返回 1 , 失败则返回0
*/
int SendH264Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, unsigned int nTimeStamp);
}
#endif


