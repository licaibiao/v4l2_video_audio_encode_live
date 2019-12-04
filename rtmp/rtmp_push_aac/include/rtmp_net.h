#ifndef __RTMP_NET_H
#define __RTMP_NET_H

int Net_Init(const char* url);
int SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp);
void Net_Close(); 

#endif
