TOP_DIR := $(shell pwd)
APP = $(TOP_DIR)/out/v4l2_yuv_h264

#CC = arm-none-linux-gnueabi-gcc
CC = gcc
#CFLAGS = -g 
LIBS = -lpthread -lx264 -lm 
#LIBS = -lpthread 
DEP_LIBS = -L$(TOP_DIR)/lib
HEADER =
OBJS = main.o  h264encoder.o

all:  $(OBJS)
	$(CC)  -o $(APP) $(OBJS) $(LIBS) 

clean:
	rm -f *.o a.out $(APP) core *~


