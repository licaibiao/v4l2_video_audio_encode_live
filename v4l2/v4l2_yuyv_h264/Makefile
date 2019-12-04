TOP_DIR := $(shell pwd)
#APP = $(TOP_DIR)x264_test
APP = x264_test

#CC = arm-none-linux-gnueabi-gcc
CC = gcc
CFLAGS = -g 
LIBS = -lpthread -lx264 -lm
#LIBS = -lpthread 
#DEP_LIBS = -L$(TOP_DIR)/lib
HEADER =
OBJS = main.o  h264encoder.o

all:  $(OBJS)
	$(CC) -g -o $(APP) $(OBJS) $(LIBS)

clean:
	rm -f *.o a.out $(APP) core *~ ./out/*


