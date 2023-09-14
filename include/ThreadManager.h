#ifndef __THREAD_MGR_H__
#define __THREAD_MHR_H__

#include <vector>
#include <thread>

class ThreadMgr
{
public:
	ThreadMgr();
private:
	std::vector<std::thread *> workers;
	
	
};


#endif