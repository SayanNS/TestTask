#include "include/shared_container.h"

SharedContainer::SharedContainer(int numThreads, int frameStride)
{
	invokeCounter = 0;
	this->frameStride = frameStride;
	this->numThreads = numThreads;
}

unsigned char *SharedContainer::getFrame()
{
	std::unique_lock<std::mutex> lk(r_mtx);

	if (++invokeCounter == numThreads)
	{
 		// The last thread notifies main thread that buffer is ready for writing
		w_cv.notify_one();
	}

	// Wait for other threads to complete processing frames to be sure that entire buffer is
	// processed and is ready for writing 
	r_cv.wait(lk); 

	// if there is no data then finish the thread
	if (numBytes == 0)
	{
		numThreads--;
		lk.unlock();

		return NULL;
	}

	numBytes -= frameStride;
	unsigned char *temp = buffer + numBytes;
	lk.unlock();

	return temp;	
}

// main thread changes read data buffer and processed data buffer
unsigned char *SharedContainer::changeBuffer(unsigned char *buffer, int numBytes)
{
	r_mtx.lock();

	// check if threads still processing data then wait for them
	if (invokeCounter != numThreads)
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
