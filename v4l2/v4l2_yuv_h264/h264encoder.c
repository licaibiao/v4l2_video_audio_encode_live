/*=============================================================================
#     FileName: h264encoder.c
#         Desc: this program aim to get image from USB camera,
#               used the V4L2 interface.
#       Author: Licaibiao
#      Version: 
#   LastChange: 2016-12-11 
#      History:
=============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./include/h264encoder.h"

void compress_begin(Encoder *en, int width, int height) {
	en->param = (x264_param_t *) malloc(sizeof(x264_param_t));
	en->picture = (x264_picture_t *) malloc(sizeof(x264_picture_t));
	x264_param_default(en->param); //set default param
	//en->param->rc.i_rc_method = X264_RC_CQP;
	// en->param->i_log_level = X264_LOG_NONE;

	 en->param->i_threads  = X264_SYNC_LOOKAHEAD_AUTO;

	en->param->i_width = width; //set frame width
	en->param->i_height = height; //set frame height

	en->param->i_frame_total = 0;

	en->param->i_keyint_max = 10;
	en->param->rc.i_lookahead = 0; 
	en->param->i_bframe = 5; 

	en->param->b_open_gop = 0;
	en->param->i_bframe_pyramid = 0;
	en->param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;

	en->param->rc.i_bitrate = 1024 * 10;//rate 10 kbps
	en->param->i_fps_num = 25; 
	en->param->i_fps_den = 1;
	x264_param_apply_profile(en->param, x264_profile_names[0]); 

	if ((en->handle = x264_encoder_open(en->param)) == 0) {
		return;
	}
	/* Create a new pic */
	x264_picture_alloc(en->picture, X264_CSP_I420, en->param->i_width,
			en->param->i_height);
	en->picture->img.i_csp = X264_CSP_I420;
	en->picture->img.i_plane = 3;
}

int compress_frame(Encoder *en, int type, uint8_t *in, uint8_t *out) {
	x264_picture_t pic_out;
	int nNal = -1;
	int result = 0;
	int i = 0;
	uint8_t *p_out = out;

	char *y = en->picture->img.plane[0];   
	char *u = en->picture->img.plane[1];   
	char *v = en->picture->img.plane[2];   

	//yuv420_length = 1.5 * en->param->i_width * en->param->i_height
	//int length =  en->param->i_width * en->param->i_height;
	//y = 640*480 ; U = 640*480*0.25, V = 640*480*0.25; 
	memcpy(y,in,307200);
	memcpy(u,in+307200,76800);
	memcpy(v,in+384000,76800);
	
	switch (type) {
	case 0:
		en->picture->i_type = X264_TYPE_P;
		break;
	case 1:
		en->picture->i_type = X264_TYPE_IDR;
		break;
	case 2:
		en->picture->i_type = X264_TYPE_I;
		break;
	default:
		en->picture->i_type = X264_TYPE_AUTO;
		break;
	}

	if (x264_encoder_encode(en->handle, &(en->nal), &nNal, en->picture,
			&pic_out) < 0) {
		return -1;
	}

	for (i = 0; i < nNal; i++) {
		memcpy(p_out, en->nal[i].p_payload, en->nal[i].i_payload);   
		p_out += en->nal[i].i_payload;								 
		result += en->nal[i].i_payload;
	}

	return result;
}

void compress_end(Encoder *en) {
	if (en->picture) {
		x264_picture_clean(en->picture);
		free(en->picture);
		en->picture = 0;
	}
	if (en->param) {
		free(en->param);
		en->param = 0;
	}
	if (en->handle) {
		x264_encoder_close(en->handle);
	}
	//free(en);
}

