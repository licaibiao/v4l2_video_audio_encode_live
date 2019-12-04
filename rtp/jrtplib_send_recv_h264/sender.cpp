/*=============================================================================  
 *     FileName: jrtplib_send.cpp  
 *         Desc: sending packets to  destination port    
 *       Author: licaibiao  
 *   LastChange: 2017-04-10  
 * =============================================================================*/
#include <rtpsession.h>
#include <rtpudpv4transmitter.h>
#include <rtpipv4address.h>
#include <rtpsessionparams.h>
#include <rtperrors.h>
#include <rtplibraryversion.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

using namespace jrtplib;

#define MAXLEN	(RTP_DEFAULTPACKETSIZE - 100)

void checkerror(int rtperr){
	if (rtperr < 0){
		std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
		exit(-1);
	}
}

class MyRTPSession : public RTPSession{
    public:  
        MyRTPSession(void);  
        ~MyRTPSession(void); 
        void SendH264Nalu(RTPSession* sess,uint8_t* m_h264Buf,int buflen);   
    protected:  
        

};

MyRTPSession::MyRTPSession(void){}  
MyRTPSession::~MyRTPSession(void){}

void MyRTPSession::SendH264Nalu(RTPSession* sess,uint8_t *h264buf,int buflen){  
    int status;  
    uint8_t *pSendbuf; 

    pSendbuf = h264buf;  
    if(buflen <= MAXLEN)
    {   
        sess->SetDefaultMark(true);  
        status = sess->SendPacket((void *)&pSendbuf[0],buflen);  
        checkerror(status);
		printf("send_packt 0 len = %d\n",buflen);  
		
    }    
    else if(buflen > MAXLEN)
    {  
        int send_packet = 0;
        int all_packet  = 0;
        int over_data   = 0;
        int iSendLen;  
		
        all_packet  = buflen / MAXLEN;  
        over_data   = buflen % MAXLEN;      
		send_packet = 0;   
        while((send_packet<all_packet) || ((send_packet ==all_packet)&&(over_data > 0)))    
        {    
			printf("send_packet = %d \n",send_packet);
            /* the first packet or the second last packet */
            if((0 == send_packet) || (send_packet<all_packet && 0!=send_packet))
            {  
        		sess->SetDefaultMark(false);  
                status = sess->SendPacket((void *)&pSendbuf[send_packet*MAXLEN],MAXLEN);  
                checkerror(status);  
				//printf("send_packet = %d \n",send_packet);
                send_packet++;  
            }  
            /* the last packet */
            else if(((all_packet==send_packet) && over_data>0) || ((send_packet== (all_packet-1))&& over_data==0))  
            {  
                sess->SetDefaultMark(true);  
               
                if ( over_data > 0)  
                {  
                    iSendLen = buflen - send_packet*MAXLEN;  
                }  
                else
                {
                     iSendLen = MAXLEN;  
                }
                   
                status = sess->SendPacket((void *)&pSendbuf[send_packet*MAXLEN],iSendLen);  
                checkerror(status);  
                send_packet++;  
				//printf("send_packet = %d \n",send_packet);
            }  
        }  
    }  
} 

int main(void)
{	
    int i;
    int num;
	int status;
    
	RTPSession sess;
    MyRTPSession sender;
	uint16_t portbase = 6666;  
    uint16_t destport = 6664;
	uint8_t destip[]={192,168,0,6};

	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;

    /* set h264 param */
    sessparams.SetUsePredefinedSSRC(true);  //设置使用预先定义的SSRC    
    sessparams.SetOwnTimestampUnit(1.0/9000.0); /* 设置采样间隔 */
    sessparams.SetAcceptOwnPackets(true);   //接收自己发送的数据包  
    
    transparams.SetPortbase(portbase);
	status = sess.Create(sessparams,&transparams);	
	checkerror(status);
	
	RTPIPv4Address addr(destip,destport);
	status = sess.AddDestination(addr);
	checkerror(status);

    sess.SetDefaultTimestampIncrement(3600);/* 设置时间戳增加间隔 */
    sess.SetDefaultPayloadType(96);
    sess.SetDefaultMark(true);    

    FILE *fd;
    int pos = 0;
    int header_flag = 0; 
    uint8_t buff[1024*100] = {0};

    fd = fopen("./test.h264","rb");
    fread(buff, 1, 4, fd);
    if((buff[0]==0)&&(buff[1]==0)&&(buff[2]==0)&&(buff[3]==1)){
        header_flag = 1;
    	pos = 4;
    }else{
        header_flag = 0;
    	pos = 3;
    }
    
    while((feof(fd)==0))
    {
        buff[pos++] = fgetc(fd);
		
        if(header_flag == 1){  //00 00 00 01
            if((buff[pos-1]==1)&&(buff[pos-2]==0)&&(buff[pos-3]==0)&&(buff[pos-4]==0)){
               	sender.SendH264Nalu(&sess, buff,pos-4);
                buff[0] = 0x00;
                buff[1] = 0x00;
                buff[2] = 0x00;
                buff[3] = 0x01;
				pos = 4;

				RTPTime::Wait(0.03);
             }
			
        }
        else{   
            if((buff[pos-1]==1)&&(buff[pos-2]==0)&&(buff[pos-3]==0)){
               	sender.SendH264Nalu(&sess, buff, pos-3);
               	buff[0] = 0x00;
               	buff[1] = 0x00;
               	buff[3] = 0x01;
               	pos = 3;

				RTPTime::Wait(0.03);
            }
		 
        }
    }
    if(pos != 0){
        sender.SendH264Nalu(&sess,buff,pos);
    }

#if 0
    for(int i = 0; i < nFrames ; i++ )  
    {  
        //读取一帧  
        read_frame_y4m(pPicIn,(hnd_t*)y4m_hnd,i);  
        if( i ==0 )  
            pPicIn->i_pts = i;  
        else  
            pPicIn->i_pts = i - 1;  
          
        //编码  
        int frame_size = x264_encoder_encode(pX264Handle,&pNals,&iNal,pPicIn,pPicOut);  
  
        if(frame_size >0)  
        {  
              
            for (int i = 0; i < iNal; ++i)  
            {//将编码数据写入文件t  
                //fwrite(pNals[i].p_payload, 1, pNals[i].i_payload, pFile);  
                //发送编码文件  
                sender.SendH264Nalu(pNals[i].p_payload,pNals[i].i_payload);  
                RTPTime::Wait(RTPTime(1,0));  
            }  
        }  
    }  
#endif
	fclose(fd);
	printf("end of the read\n");
	sess.BYEDestroy(RTPTime(10,0),0,0);    
	return 0;
}


