#ifndef __SHARED_CONTAINER__
#define __SHARED_CONTAINER__

#include <queue>
#include <stack>
#include <mutex>
#include <condition_variable>

#define MAX_POOL_SIZE 50

class SharedQueue: protected std::queue<unsigned char *>
{
public:
	void push(unsigned char *b);
	unsigned char *pop();
private:
	std::mutex rw_mtx;
	std::condition_variable cv;
};

class SharedContainer
{
public:
	SharedContainer(SharedQueue *, SharedQueue *, int);
	unsigned char *getPointer();
	void processed();
private:
	unsigned char *buffer;
	int rCounter, wCounter, numThreads;
	std::mutex mtx;
	std::condition_variable cv;
	SharedQueue *readQueue, *writeQueue;
};

class SharedStack: protected std::stack<unsigned char *>
{
public:
	SharedStack(int);
	void push(unsigned char*);
	unsigned char *pop();
private:
	int pool_count;
	int buffer_size;
};

#endif
