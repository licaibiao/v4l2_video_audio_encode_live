/*============================================================================= 
 *     FileName: ThreadBase.cpp 
 *         Desc: 
 *       Author: licaibiao 
 *   LastChange: 2017-05-3  
 * =============================================================================*/ 

#include "ThreadBase.h"

ThreadBase::ThreadBase() {
	// TODO Auto-generated constructor stub
	tid=-1;
	stopFlag=0;
	startFlag=0;
}

ThreadBase::~ThreadBase() {
	// TODO Auto-generated destructor stub

}

int ThreadBase::start(){
	int ret=pthread_create(&tid,NULL,thread_proxy_func,this);
	if(ret==0){
		startFlag=1;
//		printf("startFlag= %d\n",startFlag);
	}
	return ret;
}
int ThreadBase::stop(){
	stopFlag=1;
	return pthread_join(tid,NULL);
}
int ThreadBase::join(){
	return pthread_join(tid,NULL);
}
int ThreadBase::isStop(){
	return stopFlag;
}
int ThreadBase::isStart(){
	return startFlag;
}
void *ThreadBase::thread_proxy_func(void *args){
	ThreadBase *pThread=static_cast<ThreadBase*>(args);
//	printf("thread run\n");
	pThread->run();
	if(pThread->stopFlag==1)
		delete pThread;
	return NULL;
}
