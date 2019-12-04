#include "rtmp_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtmp/rtmp.h"   
#include "librtmp/rtmp_sys.h"   
#include "librtmp/amf.h"  

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

#ifdef WIN32     
#include <windows.h>  
#pragma comment(lib,"WS2_32.lib")   
#pragma comment(lib,"winmm.lib")  
#endif 

RTMP* m_pRtmp;  

/**
 * 初始化winsock
 *					
 * @成功则返回1 , 失败则返回相应错误代码
 */ 
int InitSockets()    
{    
#ifdef WIN32     
		WORD version;    
		WSADATA wsaData;    
		version = MAKEWORD(1, 1);    
		return (WSAStartup(version, &wsaData) == 0);    
#else     
		return TRUE;    
#endif     
}

/**
 * 初始化并连接到服务器
 *
 * @param url 服务器上对应webapp的地址
 *					
 * @成功则返回1 , 失败则返回0
 */ 
 
int Net_Init(const char* url)  
{  
	InitSockets();  

	m_pRtmp = RTMP_Alloc();
	RTMP_Init(m_pRtmp);
	//设置URL
	if (RTMP_SetupURL(m_pRtmp,(char*)url) == FALSE)
	{
		RTMP_Free(m_pRtmp);
		return -1;
	}
	//设置可写,即发布流,这个函数必须在连接前使用,否则无效
	RTMP_EnableWrite(m_pRtmp);
	//连接服务器
	if (RTMP_Connect(m_pRtmp, NULL) == FALSE) 
	{
		RTMP_Free(m_pRtmp);
		return -1;
	} 

	//连接流
	if (RTMP_ConnectStream(m_pRtmp,0) == FALSE)
	{
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		return -1;
	}
	
	return 0;  
}  

void CleanSockets()
{
	// 释放winsock  @成功则返回0 , 失败则返回相应错误代码
#ifdef WIN32     
		WSACleanup();    
#endif  
}

/**
 * 断开连接，释放相关的资源。
 *
 */    
void Net_Close() 
{  
	if(m_pRtmp)  
	{  
		RTMP_Close(m_pRtmp);  
		RTMP_Free(m_pRtmp);  
		m_pRtmp = NULL;  
	}  
	
	CleanSockets();
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

int SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp)  
{  
	int nRet =0;
	RTMPPacket* packet=NULL;
	
	//分配包内存和初始化,len为包体长度
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size);
	memset(packet,0,RTMP_HEAD_SIZE);
	
	//包体内存
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size;
	memcpy(packet->m_body,data,size);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType; //此处为类型有两种一种是音频,一种是视频
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	
	//if (RTMP_PACKET_TYPE_AUDIO ==nPacketType && size !=4)
	{
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	//packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	
	packet->m_nTimeStamp = nTimestamp;
	//发送
	
	if (RTMP_IsConnected(m_pRtmp))
	{
		nRet = RTMP_SendPacket(m_pRtmp,packet,FALSE); //TRUE为放进发送队列,FALSE是不放进发送队列,直接发送
	}
	//释放内存
	free(packet);

	return nRet;  
}  
