INCLUDES 	 = -I./include/
X264_LIBS 	 = ./lib/libx264.a 
RTMP_LIBS    = ./lib/librtmp.a 
#AAC_LIBS	 = ./lib/libfaac.a 
#AVUTIL_LIBS  = ./lib/libavutil.a   
#AVCODE_LIBS  = ./lib/libavcodec.a 
#AVDEVICE_LIBS= ./lib/libavdevice.a 
#AVFILTER_LIBS= ./lib/libavfilter.a 
#AVFORMAT_LIBS= ./lib/libavformat.a 
#POSTPROC_LIBS= ./lib/libpostproc.a 
#SWSCALE_LIBS = ./lib/libswscale.a 

LIBS         =  $(AAC_LIBS) $(X264_LIBS) $(RTMP_LIBS) $(AVUTIL_LIBS) $(AVCODE_LIBS) $(AVDEVICE_LIBS)\
				$(AVFILTER_LIBS) $(AVFORMAT_LIBS) $(POSTPROC_LIBS) $(SWSCALE_LIBS) $(SWRESAMPLE_LIBS)
COMPILE_OPTS =  $(INCLUDES)  -O2  -g
C 			 =  c
C_COMPILER   =  cc
C_FLAGS 	 =  $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS)
CPP 		 =  cpp
CPLUSPLUS_COMPILER =  c++
CPLUSPLUS_FLAGS =   $(COMPILE_OPTS) -Wall  $(CPPFLAGS) $(CXXFLAGS)
OBJ 		 =  o
LINK 		 =  c++ -o
LINK_OPTS    =  -lpthread -lssl  -lrt -g
CONSOLE_LINK_OPTS = $(LINK_OPTS)
LINK_OBJ	 = main.o librtmp_send264.o x264_encoder.o v4l2_device.o

APP = test

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

$(APP): $(LINK_OBJ)
	$(LINK) $@  $(LINK_OBJ)  $(LIBS) $(CONSOLE_LINK_OPTS)
	
clean:
	-rm -rf *.$(OBJ) $(APP) core *.core *~ *.h264

