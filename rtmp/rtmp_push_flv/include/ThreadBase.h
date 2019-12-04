/*============================================================================= 
 *     FileName: ThreadBase.h
 *         Desc: 
 *       Author: licaibiao 
 *   LastChange: 2017-05-3  
 * =============================================================================*/ 

#ifndef THREADBASE_H_
#define THREADBASE_H_

extern "C"{
#include <pthread.h>
#include <stdio.h>
}

class ThreadBase {
public:
	ThreadBase();
	virtual ~ThreadBase();

	int start();
	int stop();//只有stopFlag内嵌如run()才有效
	int isStop();
	int isStart();
	int join();
	virtual void run()=0;
private:
	static void *thread_proxy_func(void *args);
	int stopFlag;//1停止
	int startFlag;//1开始
	pthread_t tid;
};

#endif /* THREADBASE_H_ */
