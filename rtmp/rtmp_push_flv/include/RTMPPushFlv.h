/*============================================================================= 
 *     FileName: RTMPPushFlv.h
 *         Desc: 
 *       Author: licaibiao 
 *   LastChange: 2017-05-3  
 * =============================================================================*/ 

#ifndef RTMPPUSHFLV_H_
#define RTMPPUSHFLV_H_

extern "C"{
#include <stdio.h>
#include <stdlib.h>
#include "printlog.h"
}
#include <iostream>
#include <string>

#include "ThreadBase.h"
#include "librtmp/rtmp.h"
#include "librtmp/log.h"


using namespace std;

class RTMPPushFlv: public ThreadBase {
public:
	explicit RTMPPushFlv(const string url);
	virtual ~RTMPPushFlv();
	int init(const string filename);
	void run();
	void worker();
	void doPush();
	/***************************push flv********************************************/
	#define HTON16(x)  ((x>>8&0xff)|(x<<8&0xff00))
	#define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00))
	#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|\
	    (x<<8&0xff0000)|(x<<24&0xff000000))
	#define HTONTIME(x) ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00)|(x&0xff000000))

	/*read 1 byte*/
	int ReadU8(uint32_t *u8, FILE *fp) {
	    if (fread(u8, 1, 1, fp) != 1) {
	        return 0;
	    }
	    return 1;
	}

	/*read 2 byte*/
	int ReadU16(uint32_t *u16, FILE *fp) {
	    if (fread(u16, 2, 1, fp) != 1) {
	        return 0;
	    }
	    *u16 = HTON16(*u16);
	    return 1;
	}

	/*read 3 byte*/
	int ReadU24(uint32_t *u24, FILE *fp) {
	    if (fread(u24, 3, 1, fp) != 1) {
	        return 0;
	    }
	    *u24 = HTON24(*u24);
	    return 1;
	}

	/*read 4 byte*/
	int ReadU32(uint32_t *u32, FILE *fp) {
	    if (fread(u32, 4, 1, fp) != 1) {
	        return 0;
	    }
	    *u32 = HTON32(*u32);
	    return 1;
	}

	/*read 1 byte,and loopback 1 byte at once*/
	int PeekU8(uint32_t *u8, FILE *fp) {
	    if (fread(u8, 1, 1, fp) != 1) {
	        return 0;
	    }
	    fseek(fp, -1, SEEK_CUR);
	    return 1;
	}

	/*read 4 byte and convert to time format*/
	int ReadTime(uint32_t *utime, FILE *fp) {
	    if (fread(utime, 4, 1, fp) != 1) {
	        return 0;
	    }
	    *utime = HTONTIME(*utime);
	    return 1;
	}

private:
	string rtmpUrl;
	string inFile;
	FILE *fp;

	RTMP *rtmp;
	 uint32_t start_time;
	 uint32_t now_time;
    //the timestamp of the previous frame
    long pre_frame_time;
    long lasttime;
    int b_next_is_key;
    uint32_t pre_tag_size;
    char *p_file_buf;

    //read from tag header
    uint32_t type;
    uint32_t datalength;
    uint32_t timestamp;

public:
	static void test(){
		//string url("rtmp://localhost/live/test1");
		string url("rtmp://192.168.0.5:1935/live");
		string filename("test.flv");
		RTMPPushFlv *push=new RTMPPushFlv(url);
		if(push->init(filename)<0){
			log_err("Error when init");
			exit(-1);
		}
		push->doPush();
		printf("start\n");
		push->join();
	}
};

#endif /* RTMPPUSHFLV_H_ */
