APP = test
INCLUDE =  -I ./include/
LIB = ./lib/libjrtp.a ./lib/libjrtp.so ./lib/libjrtp.so.3.11.1
#LINK_OPTS = -ljrtp
LINK_OPTS = 
OBJ  = sender.cpp

out: 
	g++ -g  $(OBJ) -o $(APP)  $(LINK_OPTS) $(LIB) $(INCLUDE)

clean:
	rm -rf *o $(APP)

