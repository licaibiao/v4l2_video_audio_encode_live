INCLUDES 	 = -I./include/
RTMP_LIBS    = ./lib/librtmp.a
LIBS         = $(RTMP_LIBS)

COMPILE_OPTS = $(INCLUDES)  -O2 -g
C 			 = c
C_COMPILER   = cc
C_FLAGS 	 = $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS)
CPP 		 = cpp
CPLUSPLUS_COMPILER =    c++
CPLUSPLUS_FLAGS = $(COMPILE_OPTS) -Wall  $(CPPFLAGS) $(CXXFLAGS)
OBJ 		 =           o
LINK 		 =  c++ -o
LINK_OPTS    =  
CONSOLE_LINK_OPTS = $(LINK_OPTS) -lpthread -lssl
LINK_OBJ	 = main.o RTMPPushFlv.o sockInit.o ThreadBase.o

APP = test

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

$(APP): $(LINK_OBJ)
	$(LINK)$@  $(LINK_OBJ)  $(LIBS) $(CONSOLE_LINK_OPTS)
	
clean:
	-rm -rf *.$(OBJ) $(APP) core *.core *~ include/*~

