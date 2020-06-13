#include "shared_container.h"

SharedContainer::SharedContainer(int numThreads, int stride)
{
	invokeCounter = 0;
	this->stride = stride;
	this->numThreads = numThreads;
}

unsigned char *SharedContainer::getFrame()
{
	std::unique_lock<std::mutex> lk(r_mtx);

	if (++invokeCounter == numThreads)
	{
		w_cv.notify_one();
	}

	r_cv.wait(lk);

	if (numBytes < stride)
	{
		numThreads--;
		lk.unlock();

		return NULL;
	}

	numBytes -= stride;
	unsigned char *temp = buffer + numBytes;
	lk.unlock();

	return temp;	
}

unsigned char *SharedContainer::changeBuffer(unsigned char *buffer, int numBytes)
{
	r_mtx.lock();

	if (invokeCounter == numThreads)
	{
		r_mtx.unlock();
	}
	else
	{
		std::unique_lock<std::mutex> lk(w_mtx);
		r_mtx.unlock();
		w_cv.wait(lk);
		r_mtx.lock();
	}

	unsigned char *temp = this->buffer;
	this->buffer = buffer;
	this->numBytes = numBytes;
	invokeCounter = 0;
	r_mtx.unlock();
	r_cv.notify_all();

	return temp;
}
