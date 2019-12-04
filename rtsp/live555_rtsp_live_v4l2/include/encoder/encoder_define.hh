/*=============================================================================
 * #     FileName: 
 * #         Desc: 
 * #       Author: licaibiao
 * #      Version: 
 * #   LastChange: 2017-02-24
 * =============================================================================*/
#ifndef _ENCODER_DEFINE_HH
#define _ENCODER_DEFINE_HH


extern "C" {
#include <stdint.h>
//#include <unistd.h>
#include <x264.h>

typedef struct 
{
	x264_param_t *param;
	x264_t *handle;
	x264_picture_t *picture; 
	x264_nal_t *nal;
} Encoder;

typedef struct
{
    char *start; 
    int length;
}BUFTYPE;

}

#endif
