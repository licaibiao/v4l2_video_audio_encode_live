/*=============================================================================  
 *     FileName: x264_encoder.cpp
 *         Desc:  
 *       Author: licaibiao  
 *   LastChange: 2017-05-4   
 * =============================================================================*/  
#include "x264_encoder.h"
#include "librtmp/rtmp.h"

//void stream_stop(RTMPMOD_SPublishObj* psObj);
char* audioConfig = NULL;
long audioConfigLen = 0;
unsigned int nSendCount = 0;
unsigned char *pps = 0;
unsigned char *sps = 0;
int i_nal = 0;
int pps_len;
int sps_len;

x264_t * h = NULL;
x264_nal_t* nal_t = NULL;
x264_picture_t m_picInput;		
x264_picture_t m_picOutput;		
x264_param_t param;				
RtmpH264* pRtmpH264 = NULL;

// Class constructor
//RtmpH264::RtmpH264() : m_SwsContext(NULL),m_isCreatePublish(false)
RtmpH264::RtmpH264() : m_isCreatePublish(false)
{
	//rtmp_PublishObj.hEncoder = 0;
	rtmp_PublishObj.nSampleRate = 0;
	rtmp_PublishObj.nChannels = 0;
	rtmp_PublishObj.nTimeDelta = 0;
	rtmp_PublishObj.szPcmAudio = 0;
	rtmp_PublishObj.nPcmAudioSize = 0;
	rtmp_PublishObj.nPcmAudioLen = 0;
	rtmp_PublishObj.szAacAudio = 0;
	rtmp_PublishObj.nAacAudioSize = 0;
}

int RtmpH264::CreatePublish(char* url, int outChunkSize, int isOpenPrintLog, int logType)
{
	int rResult = 0;
	rResult = RTMP264_Connect(url, &rtmp_PublishObj.rtmp, isOpenPrintLog, logType);

	if (rResult == 1){
		m_isCreatePublish = true;
		m_startTime = RTMP_GetTime();

		//修改发送分包的大小  默认128字节
		RTMPPacket pack;
		RTMPPacket_Alloc(&pack, 4);
		pack.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
		pack.m_nChannel = 0x02;
		pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
		pack.m_nTimeStamp = 0;
		pack.m_nInfoField2 = 0;
		pack.m_nBodySize = 4;
		pack.m_body[3] = outChunkSize & 0xff; //大字节序
		pack.m_body[2] = outChunkSize >> 8;
		pack.m_body[1] = outChunkSize >> 16;
		pack.m_body[0] = outChunkSize >> 24;
		rtmp_PublishObj.rtmp->m_outChunkSize = outChunkSize;
		RTMP_SendPacket(rtmp_PublishObj.rtmp,&pack,1);
		RTMPPacket_Free(&pack);
	}

	return rResult;
}

//断开推送连接
void RtmpH264::DeletePublish()
{
	//rtmp_PublishObj.hEncoder = 0;
	rtmp_PublishObj.nSampleRate = 0;
	rtmp_PublishObj.nChannels = 0;
	rtmp_PublishObj.nTimeDelta = 0;
	rtmp_PublishObj.szPcmAudio = 0;
	rtmp_PublishObj.nPcmAudioSize = 0;
	rtmp_PublishObj.nPcmAudioLen = 0;
	rtmp_PublishObj.szAacAudio = 0;
	rtmp_PublishObj.nAacAudioSize = 0;

	if (m_isCreatePublish){
		RTMP264_Close();
	}

	m_isCreatePublish = false;
}

/*初始化视频编码器
<param name = "width">视频宽度< / param>
*<param name = "height">视频高度< / param>
*<param name = "fps">帧率< / param>
*<param name = "bitrate">比特率< / param>
*<param name = "bConstantsBitrate"> 是否恒定码率 < / param>
*/
int RtmpH264::InitVideoParams(unsigned long width, unsigned long height, unsigned long fps, unsigned long bitrate, uint32_t pixelformat, bool bConstantsBitrate = false)
{
	m_width = width;//宽，根据实际情况改
	m_height = height;//高
	m_frameRate = fps;
	m_bitRate = bitrate;
	//m_srcPicFmt=src_pix_fmt;
	m_baseFrameSize=m_width*m_height;

	x264_param_default(&param);//设置默认参数具体见common/common.c


	//* 使用默认参数，在这里因为是实时网络传输，所以使用了zerolatency的选项，使用这个选项之后就不会有delayed_frames，如果使用的不是这样的话，还需要在编码完成之后得到缓存的编码帧
	x264_param_default_preset(&param, "veryfast", "zerolatency");

	//* cpuFlags
	param.i_threads = X264_SYNC_LOOKAHEAD_AUTO; /* 取空缓冲区继续使用不死锁的保证 */
	param.i_sync_lookahead = X264_SYNC_LOOKAHEAD_AUTO;

	//* 视频选项
	param.i_width = m_width;
	param.i_height = m_height;
	param.i_frame_total = 0; 			//* 编码总帧数.不知道用0.
	param.i_keyint_min = 0;				//关键帧最小间隔
	param.i_keyint_max = (int)fps*2;	//关键帧最大间隔
	param.b_annexb = 1;					//1前面为0x00000001,0为nal长度
	param.b_repeat_headers = 0;			//关键帧前面是否放sps跟pps帧，0 否 1，放
	//param.i_csp = X264_CSP_I420;
	param.i_csp = X264_CSP_I422;

	//* B帧参数
	param.i_bframe = 0;					//B帧
	param.b_open_gop = 0;
	param.i_bframe_pyramid = 0;
	param.i_bframe_adaptive = X264_B_ADAPT_FAST;

	//* 速率控制参数
	param.rc.i_lookahead = 0;
	param.rc.i_bitrate = (int)m_bitRate; //* 码率(比特率,单位Kbps)

	if(!bConstantsBitrate){
		param.rc.i_rc_method = X264_RC_ABR;			   //参数i_rc_method表示码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
		param.rc.i_vbv_max_bitrate = (int)m_bitRate*1; // 平均码率模式下，最大瞬时码率，默认0(与-B设置相同)
	}
	else{
		param.rc.b_filler = 1;
		param.rc.i_rc_method = X264_RC_ABR;		 //参数i_rc_method表示码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
		param.rc.i_vbv_max_bitrate = m_bitRate;  // 平均码率模式下，最大瞬时码率，默认0(与-B设置相同)
		param.rc.i_vbv_buffer_size  = m_bitRate; //vbv-bufsize
	}

	//* muxing parameters
	param.i_fps_den = 1; 			// 帧率分母
	param.i_fps_num = fps;			// 帧率分子
	param.i_timebase_num = 1;
	param.i_timebase_den = 1000;

	h = x264_encoder_open(&param);	//根据参数初始化X264级别
	x264_picture_init(&m_picOutput);//初始化图片信息
	
	switch(pixelformat){
		case V4L2_PIX_FMT_YUYV:{
			x264_picture_alloc(&m_picInput, X264_CSP_I422, m_width, m_height);//图片按I422格式分配空间，最后要x264_picture_clean
			break;
		}
		
		case V4L2_PIX_FMT_YUV420:{
			x264_picture_alloc(&m_picInput, X264_CSP_I420, m_width, m_height);//图片按I420格式分配空间，最后要x264_picture_clean
			break;
		}
		
		default :{
			x264_picture_alloc(&m_picInput, X264_CSP_I420, m_width, m_height);
			break;
		}
	}
	
	m_picInput.i_pts = 0;

	i_nal = 0;
	x264_encoder_headers(h, &nal_t, &i_nal);
	if (i_nal > 0){
		for (int i = 0; i < i_nal; i++){
			//获取SPS数据，PPS数据
			if (nal_t[i].i_type == NAL_SPS){
				sps = new unsigned char[nal_t[i].i_payload - 4];
				sps_len = nal_t[i].i_payload - 4;
				memcpy(sps, nal_t[i].p_payload + 4, nal_t[i].i_payload - 4);
			}
			else if (nal_t[i].i_type == NAL_PPS){
				pps = new unsigned char[nal_t[i].i_payload - 4];;
				pps_len = nal_t[i].i_payload - 4;
				memcpy(pps, nal_t[i].p_payload + 4, nal_t[i].i_payload - 4);
			}
		}

		InitSpsPps(pps, pps_len, sps, sps_len, m_width, m_height, m_frameRate);
#if 0
		m_SwsContext= sws_getContext(m_width, m_height, /*AV_PIX_FMT_BGR24*/m_srcPicFmt, m_width, m_height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

		int ret;
		/* allocate source and destination image buffers */
		if ((ret = av_image_alloc(src_data, src_linesize,
				m_width, m_height, m_srcPicFmt, 8)) < 0) {
			fprintf(stderr, "Could not allocate source image\n");
			exit(-1);
		}
#endif
		return 1;
	}

	return 0;
}

//释放编码器内存
void RtmpH264::FreeEncodeParams()
{
	if (pps){
		delete[] pps;
		pps = NULL;
	}

	if (sps){
		delete[] sps;
		sps = NULL;
	}

	if(h){
		x264_encoder_close(h);
		h = NULL;
	}
#if 0
	if(m_SwsContext){
		av_freep(&src_data[0]);
		sws_freeContext(m_SwsContext);
		m_SwsContext = NULL;
	}
#endif
	//stream_stop(&rtmp_PublishObj);
}

/**
* 图片编码发送
*
* @成功则返回 1 , 失败则返回0
*/
int RtmpH264::SendScreenCapture(uint8_t * frame, unsigned long StrideHeight, unsigned long timespan, uint32_t pixelformat,int width, int height)
{
	int num,i;
	int index_y, index_u, index_v;
	uint8_t *y = m_picInput.img.plane[0];   
	uint8_t *u = m_picInput.img.plane[1];   
	uint8_t *v = m_picInput.img.plane[2];   
	uint8_t *in= frame;
 
	switch(pixelformat){
		case V4L2_PIX_FMT_YUYV:{
				index_y = 0;
				index_u = 0;
				index_v = 0;

				num = width * height * 2 - 4  ;
				for(i=0; i<num; i=i+4)
				{
						*(y + (index_y++)) = *(in + i);
						*(u + (index_u++)) = *(in + i + 1);
						*(y + (index_y++)) = *(in + i + 2);
						*(v + (index_v++)) = *(in + i + 3);
				}		
				break;
		}
		
		case V4L2_PIX_FMT_YUV420:{
			break;
		}
		default:{
			
			break;
		}
	}
	
	i_nal = 0;
	x264_encoder_encode(h, &nal_t, &i_nal, &m_picInput, &m_picOutput);
	m_picInput.i_pts++;//少这句的话会出现 x264 [warning]: non-strictly-monotonic PTS

	int rResult = 0;
	for (int i = 0; i < i_nal; i++)	{
		int bKeyFrame = 0;
		//memcpy(out_buf,nal_t[i].p_payload, nal_t[i].i_payload);
		//out_buf += nal_t[i].i_payload;
		//rResult += nal_t[i].i_payload;
		//获取帧数据
		if (nal_t[i].i_type == NAL_SLICE || nal_t[i].i_type == NAL_SLICE_IDR){
			if (nal_t[i].i_type == NAL_SLICE_IDR)
				bKeyFrame = 1;
             
			rResult = SendH264Packet(nal_t[i].p_payload + 4, nal_t[i].i_payload - 4, bKeyFrame, timespan);
		}
	}
	return rResult;
}

//创建推送流连接
long RTMP_CreatePublish(char* url, unsigned long outChunkSize, int isOpenPrintLog, int logType)
{
	int nResult = -1;
	if (pRtmpH264){
		pRtmpH264->FreeEncodeParams();
		pRtmpH264->DeletePublish();
		delete pRtmpH264;
	}

	pRtmpH264 = new RtmpH264();
	nResult = pRtmpH264->CreatePublish(url, (int)outChunkSize, isOpenPrintLog, logType);
	if (nResult != 1){
		pRtmpH264->m_isCreatePublish = false;
		printf(" CreatePublish ERR !!! \n ");
	}

	return nResult;
}

//断开推送流
void RTMP_DeletePublish()
{
	if (pRtmpH264){
		pRtmpH264->FreeEncodeParams();
		pRtmpH264->DeletePublish();
		delete pRtmpH264;
	}
	pRtmpH264 = NULL;
}

//初始化编码器
long RTMP_InitVideoParams(unsigned long width, unsigned long height, unsigned long fps, unsigned long bitrate, uint32_t pixelformat, bool bConstantsBitrate)
{
	int nResult = -1;
	if (!pRtmpH264){
		return -1;
	}
	
	nResult = pRtmpH264->InitVideoParams(width, height, fps, bitrate,pixelformat, bConstantsBitrate);
	if( nResult<0){
		nResult = -1;
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
		throw(-1);
	}

	if (nResult != 1){
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
	}

	return nResult;
}

//发送截图  对截图进行x264编码
long RTMP_SendScreenCapture(uint8_t *frame,  unsigned long Height, unsigned long timespan, uint32_t pixelformat, int width, int height)
{
	int nResult = -1;
	if (!pRtmpH264){
		return -1;
	}

	if (!pRtmpH264->m_isCreatePublish){
		return -1;
	}

	nResult = pRtmpH264->SendScreenCapture(frame, Height, timespan, pixelformat, width, height);
	if(nResult<0){
		nResult = -1;
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
		throw(-1);
	}

	if (nResult != 1){
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
	}

	return nResult;
}



