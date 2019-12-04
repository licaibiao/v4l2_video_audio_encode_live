INCLUDES 	 = -I./include/live555/usageEnvironment/ -I./include/live555/groupsock/ \
				-I./include/live555/liveMedia/ -I./include/live555/basicUsageEnvironment \
				-I./include/x264 -I./include/encoder
LIVE555_LIBS =  ./lib/livelib/libliveMedia.a ./lib/livelib/libgroupsock.a \
				./lib/livelib/libBasicUsageEnvironment.a ./lib/livelib/libUsageEnvironment.a
X264_LIBS 	 =  ./lib/x264lib/libx264.a ./lib/x264lib/libx264.so.148
LIBS         =  $(LIVE555_LIBS) $(X264_LIBS)

COMPILE_OPTS =      $(INCLUDES) -I. -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -g
C 			 =         c
C_COMPILER   =        cc
C_FLAGS 	 =       $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS)
CPP 		 =           cpp
CPLUSPLUS_COMPILER =    c++
CPLUSPLUS_FLAGS =   $(COMPILE_OPTS) -Wall -DBSD=1 $(CPPFLAGS) $(CXXFLAGS)
OBJ 		 =           o
LINK 		 =  c++ -o
LINK_OPTS    =  -ldl  -lm -lpthread -ldl -g
CONSOLE_LINK_OPTS = $(LINK_OPTS)
LINK_OBJ	 = H264FramedLiveSource.o H264VideoStreamer.o

APP = H264VideoStreamer

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

$(APP): $(LINK_OBJ)
	$(LINK)$@  $(LINK_OBJ)  $(LIBS) $(CONSOLE_LINK_OPTS)
	
clean:
	-rm -rf *.$(OBJ) $(APP) core *.core *~ include/*~

