/*=============================================================================  
 *     FileName: librtmp_send264.cpp
 *         Desc:  
 *       Author: licaibiao  
 *   LastChange: 2017-05-9   
 * =============================================================================*/  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtmp_send264.h"
#include "librtmp/log.h"

//定义包头长度，RTMP_MAX_HEADER_SIZE=18
#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)
//存储Nal单元数据的buffer大小
#define BUFFER_SIZE 32768
//搜寻Nal单元时的一些标志
#define GOT_A_NAL_CROSS_BUFFER 			BUFFER_SIZE+1
#define GOT_A_NAL_INCLUDE_A_BUFFER 		BUFFER_SIZE+2
#define NO_MORE_BUFFER_TO_READ 			BUFFER_SIZE+3

//网络字节序转换
char * put_byte( char *output, uint8_t nVal ){
	output[0] = nVal;
	return output+1;
}

char * put_be16(char *output, uint16_t nVal ){
	output[1] = nVal & 0xff;
	output[0] = nVal >> 8;
	return output+2;
}

char * put_be24(char *output,uint32_t nVal ){
	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output+3;
}
char * put_be32(char *output, uint32_t nVal ){
	output[3] = nVal & 0xff;
	output[2] = nVal >> 8;
	output[1] = nVal >> 16;
	output[0] = nVal >> 24;
	return output+4;
}
char *  put_be64( char *output, uint64_t nVal ){
	output=put_be32( output, nVal >> 32 );
	output=put_be32( output, nVal );
	return output;
}

char * put_amf_string( char *c, const char *str ){
	uint16_t len = strlen( str );
	c=put_be16( c, len );
	memcpy(c,str,len);
	return c+len;
}

char * put_amf_double( char *c, double d ){
	*c++ = AMF_NUMBER;  /* type: Number */
	{
		unsigned char *ci, *co;
		ci = (unsigned char *)&d;
		co = (unsigned char *)c;
		co[0] = ci[7];
		co[1] = ci[6];
		co[2] = ci[5];
		co[3] = ci[4];
		co[4] = ci[3];
		co[5] = ci[2];
		co[6] = ci[1];
		co[7] = ci[0];
	}
	return c+8;
}

void librtmpLogCallback(int level, const char *format, va_list vl){
	char* ansiStr = new char[1024];
	char* logStr = new char[1024];

	vsnprintf(ansiStr, sizeof(ansiStr) - 1, format, vl);
	ansiStr[sizeof(ansiStr) - 1] = 0;

	if (level == RTMP_LOGERROR){
		printf("librtmp Error: %s \n", ansiStr);
	}
	else{
		printf("librtmp log: %s \n", ansiStr);
	}

	delete[] ansiStr;
	delete[] logStr;
}

RTMP* m_pRtmp;
RTMPMetadata metaData;

/**
 * 初始化并连接到服务器
 * @param url 服务器上对应webapp的地址
 * @isOpenPrintLog 是否打印日志  0不打印   1打印
 * @logType 日志类型
 * @成功则返回1 , 失败则返回0
 */
int RTMP264_Connect(const char* url, RTMP** ppRtmp, int isOpenPrintLog, int logType){
	//InitSockets();

	m_pRtmp = RTMP_Alloc();
	RTMP_Init(m_pRtmp);
	m_pRtmp->Link.lFlags |= RTMP_LF_LIVE;

	if (isOpenPrintLog > 0)	{
		RTMP_LogSetCallback(librtmpLogCallback);
		switch (logType){
		case 0:
			RTMP_LogSetLevel(RTMP_LOGCRIT);
			break;
		case 1:
			RTMP_LogSetLevel(RTMP_LOGERROR);
			break;
		case 2:
			RTMP_LogSetLevel(RTMP_LOGWARNING);
			break;
		case 3:
			RTMP_LogSetLevel(RTMP_LOGINFO);
			break;
		case 4:
			RTMP_LogSetLevel(RTMP_LOGDEBUG);
			break;
		case 5:
			RTMP_LogSetLevel(RTMP_LOGDEBUG2);
			break;
		case 6:
			RTMP_LogSetLevel(RTMP_LOGALL);
			break;
		default:
			break;
		}
	}

	/*设置URL*/
	if (RTMP_SetupURL(m_pRtmp,(char*)url) == FALSE)	{
		RTMP_Free(m_pRtmp);
		return false;
	}
	/*设置可写,即发布流,这个函数必须在连接前使用,否则无效*/
	RTMP_EnableWrite(m_pRtmp);
	/*连接服务器*/
	if (RTMP_Connect(m_pRtmp, NULL) == FALSE){
		RTMP_Free(m_pRtmp);
		return -1;
	}

	/*连接流*/
	if (RTMP_ConnectStream(m_pRtmp,0) == FALSE){
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		return -1;
	}
	*ppRtmp = m_pRtmp;
	return true;
}

/**
 * 断开连接，释放相关的资源。
 */
void RTMP264_Close(){
	if(m_pRtmp)	{
		//RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
	}
	//CleanupSockets();
}

void InitSpsPps(unsigned char *pps, int pps_len, unsigned char * sps, int sps_len, int width, int height, int fps){
	memset(&metaData, 0, sizeof(RTMPMetadata));
	metaData.nWidth 	= width;
	metaData.nHeight 	= height;
	metaData.nFrameRate = fps;
	metaData.nSpsLen 	= sps_len;
	metaData.Sps 		= sps;
	metaData.nPpsLen 	= pps_len;
	metaData.Pps 		= pps;
}

/**
 * 发送RTMP数据包
 *
 * @param nPacketType 数据类型
 * @param data 存储数据内容
 * @param size 数据大小
 * @param nTimestamp 当前包的时间戳
 *
 * @成功则返回 1 , 失败则返回一个小于0的数
 */
int SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp){
	RTMPPacket* packet;
	int nRet =0;

	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size);
	memset(packet,0,RTMP_HEAD_SIZE);

	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size;
	memcpy(packet->m_body,data,size);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType; 				/*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO ==nPacketType && size !=4){
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = nTimestamp;

	if (RTMP_IsConnected(m_pRtmp)){
		nRet = RTMP_SendPacket(m_pRtmp,packet,TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}

	free(packet);
	return nRet;
}

/**
 * 发送视频的sps和pps信息
 * @param pps 存储视频的pps信息
 * @param pps_len 视频的pps信息长度
 * @param sps 存储视频的pps信息
 * @param sps_len 视频的sps信息长度
 *
 * @成功则返回 1 , 失败则返回0
 */
int SendVideoSpsPps(unsigned char *pps,int pps_len,unsigned char * sps,int sps_len){
	RTMPPacket * packet=NULL;		//rtmp包结构
	unsigned char * body=NULL;
	int i;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);

	memset(packet,0,RTMP_HEAD_SIZE+1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i],sps,sps_len);
	i +=  sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i],pps,pps_len);
	i +=  pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;

	int nRet = RTMP_SendPacket(m_pRtmp,packet,TRUE);
	free(packet);    
	return nRet;
}

/**
 * 发送H264数据帧
 * @param data 存储数据帧内容
 * @param size 数据帧的大小
 * @param bIsKeyFrame 记录该帧是否为关键帧
 * @param nTimeStamp 当前帧的时间戳
 *
 * @成功则返回 1 , 失败则返回0
 */
int SendH264Packet(unsigned char *data,unsigned int size,int bIsKeyFrame,unsigned int nTimeStamp){
	int i = 0;
	if(data == NULL && size<11){
		return false;
	}

	unsigned char *body = (unsigned char*)malloc(size+9);
	memset(body,0,size+9);

	if(bIsKeyFrame){
		body[i++] = 0x17;// 1:Iframe  7:AVC
		body[i++] = 0x01;// AVC NALU
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;


		// NALU size
		body[i++] = size>>24 &0xff;
		body[i++] = size>>16 &0xff;
		body[i++] = size>>8 &0xff;
		body[i++] = size&0xff;
		// NALU data
		memcpy(&body[i],data,size);
		int bRetSpsPps = SendVideoSpsPps(metaData.Pps,metaData.nPpsLen,metaData.Sps,metaData.nSpsLen);

		int bRet = 0;
		if (bRetSpsPps > 0){
			bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, nTimeStamp);
		}
		free(body);
		return bRet;
	}
	else{
		body[i++] = 0x27;// 2:Pframe  7:AVC
		body[i++] = 0x01;// AVC NALU
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;

		// NALU size
		body[i++] = size>>24 &0xff;
		body[i++] = size>>16 &0xff;
		body[i++] = size>>8 &0xff;
		body[i++] = size&0xff;
		// NALU data
		memcpy(&body[i],data,size);
		int bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, nTimeStamp);
		free(body);
		return bRet;
	}
}


