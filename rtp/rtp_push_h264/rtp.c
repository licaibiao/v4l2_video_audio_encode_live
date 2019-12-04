/*============================================================================= 
 *     FileName: rtp.c 
 *         Desc: rtp payload h.264 data 
 *       Author: licaibiao 
 *   LastChange: 2017-04-07  
 * =============================================================================*/
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <memory.h>    
#include <sys/types.h>  
#include <sys/socket.h>  
#include <arpa/inet.h>
#include <netinet/in.h>  
#include <netdb.h> 
#include <unistd.h> 
#include "rtp.h"  

//#define DEBUG_LOG

typedef struct  
{  
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)  
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)  
	unsigned max_size;            //! Nal Unit Buffer size  
	int forbidden_bit;            //! should be always FALSE  
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx  
	int nal_unit_type;            //! NALU_TYPE_xxxx      
	char *buf;                    //! contains the first byte followed by the EBSP  
	unsigned short lost_packets;  //! true, if packet loss is detected  
} NALU_t;  

FILE *bits = NULL;              //!< the bit stream file  
static int FindStartCode2 (unsigned char *Buf);//查找开始字符0x000001  
static int FindStartCode3 (unsigned char *Buf);//查找开始字符0x00000001  
static int info2=0;
static int info3=0; 
RTP_FIXED_HEADER *rtp_hdr;  
NALU_HEADER      *nalu_hdr;  
FU_INDICATOR     *fu_ind;  
FU_HEADER        *fu_hdr;  

NALU_t *AllocNALU(int buffersize)  
{  
	NALU_t *n;  
	if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)  
	{  
		printf("AllocNALU: n");  
		exit(0);  
	}  
	n->max_size = buffersize;  
	if((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)  
	{  
		free (n);  
		printf ("AllocNALU: n->buf");  
		exit(0);  
	}  
	return n;  
}  

void FreeNALU(NALU_t *n)  
{  
	if (n)  
	{  
		if (n->buf)  
		{  
			free(n->buf);  
			n->buf=NULL;  
		}  
		free (n);  
	}  
}  

void OpenBitstreamFile (char *fn)  
{  
	if (NULL == (bits = fopen(fn, "rb")))  
	{  
		printf("open file error\n");  
		exit(0);  
	}  
}  

//这个函数输入为一个NAL结构体，主要功能为得到一个完整的NALU并保存在NALU_t的buf中，获取他的长度，填充F,IDC,TYPE位。  
//并且返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
//前缀之后的第一个字节为 NALU_HEADER   
int GetAnnexbNALU (NALU_t *nalu)  
{  
	int pos = 0;  
	int rewind; 
	int StartCodeFound;
	unsigned char *Buf;  
	
	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)
		printf ("GetAnnexbNALU: Could not allocate Buf memory\n");  
	
	printf("nalu->max_size=%d\n",(int)nalu->max_size);
	memset(Buf,0,nalu->max_size);
	nalu->startcodeprefix_len = 3;//初始化码流序列的开始字符为3个字节  
	if (3 != fread (Buf, 1, 3, bits))//从码流中读3个字节  
	{  
		free(Buf);  
		return 0;  
	}  
	info2 = FindStartCode2 (Buf);//判断是否为0x000001   
	if(info2 != 1)   
	{  
		//如果不是，再读一个字节  
		if(1 != fread(Buf+3, 1, 1, bits))//读一个字节  
		{  
			free(Buf);  
			return 0;  
		}  
		info3 = FindStartCode3 (Buf);//判断是否为0x00000001  
		if (info3 != 1)//如果不是，返回-1  
		{   
			free(Buf);  
			return -1;  
		}  
		else   
		{  
			//如果是0x00000001,得到开始前缀为4个字节  
			pos = 4;  
			nalu->startcodeprefix_len = 4;  
		}  
	}   
	else  
	{  
		//如果是0x000001,得到开始前缀为3个字节  
		nalu->startcodeprefix_len = 3;  
		pos = 3;  
	}  
	//查找下一个开始字符的标志位  
	StartCodeFound = 0;  
	info2 = 0;  
	info3 = 0;      
	
	while (!StartCodeFound)  
	{  
		if (feof (bits))//判断是否到了文件尾  
		{  
			nalu->len = (pos-1) - nalu->startcodeprefix_len;  
			printf("nalu->len1=%d\n",nalu->len );
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len); //拷贝一个完整NALU，不拷贝起始前缀0x000001或0x00000001      
			nalu->forbidden_bit 	= nalu->buf[0] & 0x80;     // 1 bit
			nalu->nal_reference_idc = nalu->buf[0] & 0x60;     // 2 bit  
			nalu->nal_unit_type		= nalu->buf[0] & 0x1f;     // 5 bit  
			free(Buf);  
			return pos - 1;  
		}  
		Buf[pos++] = fgetc (bits);//读一个字节到BUF中  
		info3 = FindStartCode3(&Buf[pos-4]);//判断是否为0x00000001  
		if(info3 != 1) 
		{
			info2 = FindStartCode2(&Buf[pos-3]);//判断是否为0x000001  	
		}	
		StartCodeFound = (info2 == 1 || info3 == 1);  
	}  
	
	// Here, we have found another start code (and read length of startcode bytes more than we should  
	// have.  Hence, go back in the file  
	rewind = (info3 == 1) ? -4 : -3;  
	
	if (0 != fseek (bits, rewind, SEEK_CUR))//把文件指针指向前一个NALU的末尾  
	{  
		free(Buf);  
		printf("GetAnnexbNALU: Cannot fseek in the bit stream file");  
	}  
	
	// Here the Start code, the complete NALU, and the next start code is in the Buf.    
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next  
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code  
	
	nalu->len = (pos+rewind) - nalu->startcodeprefix_len;  
	printf("nalu->len2=%d\n",nalu->len );
	memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//拷贝一个完整NALU，不拷贝起始前缀0x000001或0x00000001  
	nalu->forbidden_bit		 = nalu->buf[0] & 0x80; 	// 1  bit
	nalu->nal_reference_idc  = nalu->buf[0] & 0x60;     // 2  bit  
	nalu->nal_unit_type		 = nalu->buf[0] & 0x1f;     // 5  bit  
	free(Buf);  
	return (pos+rewind);//返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度  
}  

//输出NALU长度和TYPE  
void dump(NALU_t *n)  
{  
    if (!n)return;  
    //printf("a new nal:");  
    printf(" len: %d  ", n->len);  
    printf("nal_unit_type: %x\n", n->nal_unit_type);  
}  

int main(int argc, char* argv[])  
{  
    //FILE *stream;  
    //stream=fopen("Test.264", "wb");  
    NALU_t *n;
    SOCKET socket1;    
    char  *nalu_payload;    
    char  sendbuf[1500];
    int   len ; 
    int   bytes = 0;  
    float framerate = 25; 
    unsigned short seq_num = 0;  
    unsigned int timestamp_increase = 0;
	unsigned int ts_current = 0;  
    struct sockaddr_in server;  

    len = sizeof(server);
    OpenBitstreamFile("./h264/test.h264");
    timestamp_increase = (unsigned int)(90000.0 / framerate); //+0.5);  
    server.sin_family = AF_INET;  
    server.sin_port = htons(DEST_PORT);            
    server.sin_addr.s_addr = inet_addr(DEST_IP);   
    socket1 = socket(AF_INET, SOCK_DGRAM, 0);  

    n = AllocNALU(8000000);//为结构体nalu_t及其成员buf分配空间。返回值为指向nalu_t存储空间的指针  

    while(!feof(bits))   
    {  
        GetAnnexbNALU(n);//每执行一次，文件的指针指向本次找到的NALU的末尾，下一个位置即为下个NALU的起始码0x000001  
        dump(n);//输出NALU长度和TYPE  
		//（1）一个NALU就是一个RTP包的情况： RTP_FIXED_HEADER（12字节）  + NALU_HEADER（1字节） + EBPS  
        //（2）一个NALU分成多个RTP包的情况： RTP_FIXED_HEADER （12字节） + FU_INDICATOR （1字节）+  FU_HEADER（1字节） + EBPS(1400字节)  
        memset(sendbuf, 0, 1500);//清空sendbuf；此时会将上次的时间戳清空，因此需要ts_current来保存上次的时间戳值  
		//rtp固定包头，为12字节,该句将sendbuf[0]的地址赋给rtp_hdr，以后对rtp_hdr的写入操作将直接写入sendbuf。  
        rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0];   
        //设置RTP HEADER，  
        rtp_hdr->csrc_len	 = 0;
		//rtp_hdr->extension=0;
		//rtp_hdr->padding=0;
        rtp_hdr->payload     = H264;  		//负载类型号，  
        rtp_hdr->version     = 2;  			//版本号，此版本固定为2  
        rtp_hdr->marker      = 0;   		//标志位，由具体协议规定其值。  
        rtp_hdr->ssrc        = htonl(10); 	//随机指定为10，并且在本RTP会话中全局唯一  bytes 8-11
		//  当一个NALU小于1400字节的时候，采用一个单RTP包发送  
        if(n->len<=1400)  
        {     
            //设置rtp M 位；  
            rtp_hdr->marker = 1;  
            rtp_hdr->seq_no = htons(seq_num ++); //序列号，每发送一个RTP包增1  bytes 2, 3
            //设置NALU HEADER,并将这个HEADER填入sendbuf[12]  
            nalu_hdr 		= (NALU_HEADER*)&sendbuf[12]; //将sendbuf[12]的地址赋给nalu_hdr，之后对nalu_hdr的写入就将写入sendbuf中；  
            nalu_hdr->F 	= n->forbidden_bit >> 7;  
            nalu_hdr->NRI 	= n->nal_reference_idc >> 5;//有效数据在n->nal_reference_idc的第6，7位，需要右移5位才能将其值赋给nalu_hdr->NRI。  
            nalu_hdr->TYPE	= n->nal_unit_type;  

            nalu_payload	= &sendbuf[13];//同理将sendbuf[13]赋给nalu_payload  
            memcpy(nalu_payload, n->buf + 1, n->len - 1);//去掉nalu头的nalu剩余内容写入sendbuf[13]开始的字符串。

            ts_current = ts_current + timestamp_increase;  
			printf("ts_current=%d\n",ts_current);
            rtp_hdr->timestamp = htonl(ts_current);
			printf("ts_current1=%x\n",rtp_hdr->timestamp);
            bytes = n->len + 12+1;                      //获得sendbuf的长度,为nalu的长度（包含NALU头但除去起始前缀）加上rtp_header的固定长度12字节  
#ifdef DEBUG_LOG
			{
				int ii = 0;
				printf("-----------------------------------\n");
				for(ii=0; ii<22; ii++)
				{
					printf("buf%d=%x  ", ii, (unsigned char)sendbuf[ii]);
				}
				printf("\n");
				printf("------------------------------------\n");
			}
#endif			
			sendto(socket1, sendbuf, bytes, 0, (struct sockaddr *)&server, sizeof(server));
            usleep(40000);  
			//fwrite(sendbuf,bytes, 1, stream);   
        }  
        else if(n->len>1400)  
        {  
            //得到该nalu需要用多少长度为1400字节的RTP包来发送  
            int k = 0;
			int l = 0; 
			int t = 0;			//用于指示当前发送的是第几个分片RTP包  			
            k = n->len / 1400;	//需要k个1400字节的RTP包  
            l = n->len % 1400;	//最后一个RTP包的需要装载的字节数  
           
            ts_current = ts_current+timestamp_increase;  
			printf("ts_current=%d\n",ts_current);
            rtp_hdr->timestamp = htonl(ts_current);  
            while(t <= k)  
            {  
                rtp_hdr->seq_no = htons(seq_num++); //序列号，每发送一个RTP包增1  
                if(!t)								//发送一个需要分片的NALU的第一个分片，置FU HEADER的S位  
                {  
                    //设置rtp M 位；  
                    rtp_hdr->marker = 0;  
                    //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]  
                    fu_ind 		 = (FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；  
                    fu_ind->F 	 = n->forbidden_bit >> 7;  
                    fu_ind->NRI	 = n->nal_reference_idc >> 5;  
                    fu_ind->TYPE = 28;                     
                    //设置FU HEADER,并将这个HEADER填入sendbuf[13]  
                    fu_hdr 		 = (FU_HEADER*)&sendbuf[13];  
                    fu_hdr->E	 = 0;  
                    fu_hdr->R	 = 0;  
                    fu_hdr->S	 = 1;  
                    fu_hdr->TYPE = n->nal_unit_type;  
                    nalu_payload = &sendbuf[14];			//同理将sendbuf[14]赋给nalu_payload  
                    memcpy(nalu_payload, n->buf + 1, 1400);//去掉NALU头  
                    bytes = 1400 + 12 + 2;                 //获得sendbuf的长度,为nalu的长度（除去起始前缀和NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节  
#ifdef DEBUG_LOG
                    {
						int ii=0;
						printf("-----------------------------------\n");
						for(ii=0;ii<22;ii++)
						{
							printf("buf%d=%x  ",ii,(unsigned char)sendbuf[ii]);
						}
						printf("\n");
						printf("------------------------------------\n");
					}	
#endif
					sendto(socket1, sendbuf, bytes, 0, (struct sockaddr *)&server, sizeof(server));
					//fwrite(sendbuf,bytes, 1, stream);  
					//usleep(20000);  
                    t++;                      
                }  
                //发送一个需要分片的NALU的非第一个分片，清零FU HEADER的S位，如果该分片是该NALU的最后一个分片，置FU HEADER的E位  
                else if(k == t)//发送的是最后一个分片，注意最后一个分片的长度可能超过1400字节（当l>1386时）。  
                {  
                    //设置rtp M 位；当前传输的是最后一个分片时该位置1  
                    rtp_hdr->marker = 1;  
					
                    //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]  
                    fu_ind			= (FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；  
                    fu_ind->F		= n->forbidden_bit >> 7;  
                    fu_ind->NRI		= n->nal_reference_idc >> 5;  
                    fu_ind->TYPE	= 28;  
					
                    //设置FU HEADER,并将这个HEADER填入sendbuf[13]  
                    fu_hdr 			= (FU_HEADER*)&sendbuf[13];  
                    fu_hdr->R       = 0;  
                    fu_hdr->S 		= 0;  
                    fu_hdr->TYPE	= n->nal_unit_type; 
                    fu_hdr->E		= 1;   
                    nalu_payload	= &sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload  
					
					if((n != NULL) && (n->buf != NULL) && (l > 1))
					{
						memcpy(nalu_payload, n->buf + t * 1400 + 1, l - 1);//将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。  
						bytes = l - 1 + 12 + 2;       //获得sendbuf的长度,为剩余nalu的长度l-1加上rtp_header，FU_INDICATOR,FU_HEADER三个包头共14字节  
#ifdef DEBUG_LOG
						{
							int ii=0;
							printf("-----------------------------------\n");
							for(ii=0;ii<22;ii++)
							{
								printf("buf%d=%x  ",ii,(unsigned char)sendbuf[ii]);
							}
							printf("\n");
							printf("------------------------------------\n");
						}
#endif	
						sendto(socket1, sendbuf, bytes, 0, (struct sockaddr *)&server, sizeof(server));						
					}
					else
					{
						printf("n->buf == NULL !\n");
					}                  
					//fwrite(sendbuf,bytes, 1, stream);  
					//usleep(20000);
                    t++;                     
                }  
                else if(t < k && 0 != t)  
                {  
                    //设置rtp M 位；  
                    rtp_hdr->marker = 0;  
                    //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]  
                    fu_ind 			= (FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；  
                    fu_ind->F 		= n->forbidden_bit>>7;  
                    fu_ind->NRI 	= n->nal_reference_idc>>5;  
                    fu_ind->TYPE 	= 28;                           
                    //设置FU HEADER,并将这个HEADER填入sendbuf[13]  
                    fu_hdr 			= (FU_HEADER*)&sendbuf[13];  
                    //fu_hdr->E=0;  
                    fu_hdr->R 		= 0;  
                    fu_hdr->S 		= 0;  
                    fu_hdr->E 		= 0;  
                    fu_hdr->TYPE 	= n->nal_unit_type;                    
                    nalu_payload 	= &sendbuf[14];				  //同理将sendbuf[14]的地址赋给nalu_payload  
                    memcpy(nalu_payload, n->buf + t * 1400 + 1, 1400);//去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。  
                    bytes = 1400 + 12 + 2;                      //获得sendbuf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节  
#ifdef DEBUG_LOG
					{
						int ii = 0;
						printf("-----------------------------------\n");
						for(ii=0;ii<22;ii++)
						{
							printf("buf%d=%x  ",ii,(unsigned char)sendbuf[ii]);
						}
						printf("\n");
						printf("------------------------------------\n");
					}	
#endif
					sendto(socket1, sendbuf, bytes, 0, (struct sockaddr *)&server, sizeof(server));				
					//fwrite(sendbuf, bytes, 1, stream);  
                    //usleep(20000);  
                    t++;  
                }  
            }  
			usleep(40000);  
        }     
    }  
    FreeNALU(n);  
    fclose(bits);
    bits = NULL;
    //fclose(stream);
    return 0;  
}  

static int FindStartCode2 (unsigned char *Buf)  
{  
 if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //判断是否为0x000001,如果是返回1  
 else return 1;  
}  

static int FindStartCode3 (unsigned char *Buf)  
{  
 if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//判断是否为0x00000001,如果是返回1  
 else return 1;  
} 



