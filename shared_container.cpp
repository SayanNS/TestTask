#include "shared_container.h"

SharedContainer::SharedContainer(SharedQueue *readQueue, SharedQueue *writeQueue, int numThreads)
{
	this->wCounter = 0;
	this->rCounter = numThreads;
	this->numThreads = numThreads;
	this->readQueue = readQueue;
	this->writeQueue = writeQueue;
}

unsigned char *SharedContainer::getPointer()
{
	mtx.lock();

	if (rCounter == numThreads)
	{
		buffer = readQueue->pop();

		if (buffer == NULL)
		{
			writeQueue->push(NULL);
		}

		rCounter = 0;
	}

	rCounter++;
	unsigned char *pointer = buffer;
	mtx.unlock();

	return pointer;
}

void SharedContainer::processed()
{
	std::unique_lock<std::mutex> lk(mtx);

	if (++wCounter == numThreads)
	{
		writeQueue->push(buffer);
		wCounter = 0;
		lk.unlock();
		cv.notify_all();
	}
	else
	{
		cv.wait(lk);
		lk.unlock();
	}
}

void SharedQueue::push(unsigned char* buffer)
{
	rw_mtx.lock();

	if (empty())
	{
		queue::push(buffer);
		rw_mtx.unlock();
		cv.notify_one();
	}
	else
	{
		queue::push(buffer);
		rw_mtx.unlock();
	}
}

unsigned char *SharedQueue::pop()
{
	std::unique_lock<std::mutex> lk(rw_mtx);

	if (empty())
	{
		cv.wait(lk);
	}

	unsigned char *buffer = front();
	queue::pop();
	lk.unlock();

	return buffer;
}

SharedStack::SharedStack(int buffer_size)
{
	pool_count = 0;

	this->buffer_size = buffer_size;
}

void SharedStack::push(unsigned char *buffer)
{
	stack::push(buffer);
}

unsigned char* SharedStack::pop()
{
	if (empty())
	{
		if (pool_count == MAX_POOL_SIZE)
		{
			return nullptr;
		}

		pool_count++;

		return new unsigned char[buffer_size];
	}

	unsigned char *buffer = top();
	stack::pop();

	return buffer;
}
