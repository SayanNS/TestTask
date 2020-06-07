#include "shared_container.h"

void SharedContainer::push(unsigned char* buffer)
{
	rw_mtx.lock();

	if (queue.empty())
	{
		queue.push(buffer);
		rw_mtx.unlock();
		cv.notify_one();
	}
	else
	{
		queue.push(buffer);
		rw_mtx.unlock();
	}
}

unsigned char* SharedContainer::pop(bool wait)
{
	std::unique_lock<std::mutex> lk(rw_mtx);

	if (wait)
	{
		if (queue.empty())
		{
			cv.wait(lk);
		}

		unsigned char *buffer = queue.front();
		queue.pop();
		lk.unlock();

		return buffer;
	}
	else
	{
		if (queue.empty())
		{
			lk.unlock();
			return NULL;
		}

		unsigned char *buffer = queue.front();

		if (buffer)
		{
			queue.pop();
		}

		lk.unlock();

		return buffer;
	}
}

SharedPool::SharedPool(int buffer_size)
{
	pool_count = 0;

	this->buffer_size = buffer_size;
}

void SharedPool::push(unsigned char *buffer)
{
	stack.push(buffer);
}

unsigned char* SharedPool::pop()
{
	if (stack.empty())
	{
		if (pool_count == MAX_POOL_SIZE)
		{
			return NULL;
		}

		pool_count++;

		return new unsigned char[buffer_size];
	}

	unsigned char *buffer = stack.top();
	stack.pop();

	return buffer;
}
