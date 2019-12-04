/*============================================================================= 
 *     FileName: rtp.h 
 *         Desc: rtp payload h.264 data
 *       Author: licaibiao 
 *   LastChange: 2017-04-07  
 * =============================================================================*/
#include <stdio.h>  
#include <stdlib.h>   
#include <string.h>    
#include <sys/socket.h>  
  
#define PACKET_BUFFER_END      (unsigned int)0x00000000   
#define MAX_RTP_PKT_LENGTH     1400  
#define DEST_IP                "192.168.0.108" /* 显示端 IP 地址 */ 
#define DEST_PORT              6666 
#define H264                   96  
  
typedef int SOCKET;  

/******************************************************************
RTP_FIXED_HEADER
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier            |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|            contributing source (CSRC) identifiers             |
|                             ....                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

******************************************************************/
typedef struct   
{  
    /* byte 0 */  
    unsigned char csrc_len:4;       /* expect 0 */  
    unsigned char extension:1;      /* expect 1, see RTP_OP below */  
    unsigned char padding:1;        /* expect 0 */  
    unsigned char version:2;        /* expect 2 */  
    /* byte 1 */  
    unsigned char payload:7;        /* RTP_PAYLOAD_RTSP */  
    unsigned char marker:1;         /* expect 1 */  
    /* bytes 2, 3 */  
    unsigned short seq_no;              
    /* bytes 4-7 */  
    unsigned  int timestamp;      /* !!Can not use long type,long = 8 Byte int 64 bit  system!!*/         
    /* bytes 8-11 */  
    unsigned int ssrc;            /* stream number is used here. */  
} RTP_FIXED_HEADER;  



/******************************************************************
NALU_HEADER
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
******************************************************************/ 
typedef struct {  
    //byte 0  
    unsigned char TYPE:5;  
    unsigned char NRI:2;  
    unsigned char F:1;      
           
} NALU_HEADER;/* 1 BYTES */  



/******************************************************************
FU_INDICATOR
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
******************************************************************/ 
typedef struct {  
    //byte 0  
    unsigned char TYPE:5;  
    unsigned char NRI:2;   
    unsigned char F:1;              
} FU_INDICATOR; /* 1 BYTES */  



/******************************************************************
FU_HEADER
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|S|E|R|  Type   |
+---------------+
******************************************************************/
typedef struct {  
    //byte 0  
    unsigned char TYPE:5;  
    unsigned char R:1;  
    unsigned char E:1;  
    unsigned char S:1;      
} FU_HEADER;	/* 1 BYTES */  


