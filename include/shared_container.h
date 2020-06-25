#ifndef __SHARED_CONTAINER__
#define __SHARED_CONTAINER__

#include <mutex>
#include <condition_variable>

class SharedContainer
{
public:
	SharedContainer(int, int);
	unsigned char *changeBuffer(unsigned char *, int);
	unsigned char *getFrame();
private:
	unsigned char *buffer;
	int frameStride, numThreads, invokeCounter, numBytes;
	std::mutex r_mtx, w_mtx;
	std::condition_variable r_cv, w_cv;
};

#endif
