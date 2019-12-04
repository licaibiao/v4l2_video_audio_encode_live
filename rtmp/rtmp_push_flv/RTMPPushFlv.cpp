/*============================================================================= 
 *     FileName: RTMPPushFlv.cpp 
 *         Desc: 
 *       Author: licaibiao 
 *   LastChange: 2017-05-3  
 * =============================================================================*/ 

#include "RTMPPushFlv.h"
#include "sockInit.h"
RTMPPushFlv::RTMPPushFlv(const string url) {
	// TODO Auto-generated constructor stub
	rtmpUrl=url;
	fp=NULL;
	 start_time = 0;
	 now_time = 0;

	 pre_frame_time = 0;
	 lasttime = 0;
	 b_next_is_key = 1;
	 pre_tag_size = 0;

	 type = 0;
	 datalength = 0;
	 timestamp = 0;

	 rtmp = RTMP_Alloc();
}

RTMPPushFlv::~RTMPPushFlv() {
	// TODO Auto-generated destructor stub
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	CleanupSockets();
	if (rtmp != NULL) {
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		rtmp = NULL;
	}
	if (p_file_buf != NULL) {
		free(p_file_buf);
		p_file_buf = NULL;
	}
}

int RTMPPushFlv::init(const string filename){
	inFile=filename;
	 fp = fopen(inFile.c_str(), "rb");
	if (NULL == fp) {
		log_err("Open File Error");
		return -1;
	}
	InitSockets();
	RTMP_Init(rtmp);
	//set connection timeout,default 30s
	rtmp->Link.timeout = 5;
	if (!RTMP_SetupURL(rtmp, const_cast<char*>(rtmpUrl.c_str()))) {
		RTMP_Log(RTMP_LOGERROR, "SetupURL Err\n");
		//RTMP_Free(rtmp);
		return -1;
	}
	RTMP_EnableWrite(rtmp);
	if (!RTMP_Connect(rtmp, NULL)) {
		RTMP_Log(RTMP_LOGERROR, "Connect Err\n");
		//RTMP_Free(rtmp);
		return -1;
	}

	if (!RTMP_ConnectStream(rtmp, 0)) {
		RTMP_Log(RTMP_LOGERROR, "ConnectStream Err\n");
		//RTMP_Close(rtmp);
		//RTMP_Free(rtmp);
		return -1;
	}


	//jump over FLV Header
	fseek(fp, 9, SEEK_SET);
	//jump over previousTagSizen
	fseek(fp, 4, SEEK_CUR);
	return 0;
}

void RTMPPushFlv::run(){
	worker();
}
void RTMPPushFlv::worker(){
	log_info("Start to send data ...");
	start_time = RTMP_GetTime();
	while (1) {
		if ((((now_time = RTMP_GetTime()) - start_time)
				< (pre_frame_time)) && b_next_is_key) {
			//wait for 1 sec if the send process is too fast
			//this mechanism is not very good,need some improvement
			if (pre_frame_time > lasttime) {
				RTMP_LogPrintf("TimeStamp:%8lu ms\n", pre_frame_time);
				lasttime = pre_frame_time;
			}
			sleep(1);
			continue;
		}

		//jump over type
		fseek(fp, 1, SEEK_CUR);
		if (!ReadU24(&datalength, fp)) {
			break;
		}
		if (!ReadTime(&timestamp, fp)) {
			break;
		}
		//jump back
		fseek(fp, -8, SEEK_CUR);

		//len = PreviousTagSize 4 + TagHeader 11 + datalength 
		p_file_buf = (char *) malloc(4 + 11 + datalength);
		memset(p_file_buf, 0, 4 + datalength + 11);
		if (fread(p_file_buf, 1, 4 + datalength + 11, fp) != (4 + datalength + 11)) {
			break;
		}

		pre_frame_time = timestamp;

		if (!RTMP_IsConnected(rtmp)) {
			RTMP_Log(RTMP_LOGERROR, "rtmp is not connect\n");
			break;
		}
		if (!RTMP_Write(rtmp, p_file_buf, 11 + datalength + 4)) {
			RTMP_Log(RTMP_LOGERROR, "Rtmp Write Error\n");
			break;
		}

		free(p_file_buf);
		p_file_buf = NULL;

		if (!PeekU8(&type, fp)) {
			break;
		}
		if (0x09 == type) {   //video
			if (fseek(fp, 11, SEEK_CUR) != 0) {
				break;
			}
			if (!PeekU8(&type, fp)) {
				break;
			}
			if (type == 0x17) {  /*this is the key frame of the video */
				b_next_is_key = 1;
			} else {
				b_next_is_key = 0;
			}
			fseek(fp, -11, SEEK_CUR);
		}
	}
	log_info("Send Data Over");
}
void RTMPPushFlv::doPush(){
	this->start();
}
