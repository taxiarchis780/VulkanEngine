#include <ThreadManager.h>


ThreadMgr::ThreadMgr()
{
	workers.resize(2);
	int index = 0;
	for (auto &worker : workers)
	{
		worker = new std::thread();
		index++;
	}
}