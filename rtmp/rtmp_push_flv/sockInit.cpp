/*============================================================================= 
 *     FileName: sockInit.cpp 
 *         Desc: 
 *       Author: licaibiao 
 *   LastChange: 2017-05-3  
 * =============================================================================*/ 


#ifdef WIN32
#include <windows.h>
#pragma comment(lib,"WS2_32.lib")
#pragma comment(lib,"winmm.lib")
#endif
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
		return 1;
	#endif
}

/**
 * 释放winsock
 *
 * @成功则返回0 , 失败则返回相应错误代码
 */
void CleanupSockets()
{
	#ifdef WIN32
		WSACleanup();
	#endif
}

