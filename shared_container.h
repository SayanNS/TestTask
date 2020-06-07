#ifndef __SHARED_CONTAINER__
#define __SHARED_CONTAINER__

#include <queue>
#include <stack>
#include <mutex>
#include <condition_variable>

#define MAX_POOL_SIZE 30

class SharedContainer
{
public:
	void push(unsigned char* buffer);
	unsigned char* pop(bool wait);
private:
	std::queue<unsigned char*> queue;
	std::mutex rw_mtx;
	std::condition_variable cv;
};

class SharedPool
{
public:
	SharedPool(int buffer_size);
	void push(unsigned char *buffer);
	unsigned char* pop();
private:
	int pool_count;
	int buffer_size;
	std::stack<unsigned char*> stack;
};

#endif
