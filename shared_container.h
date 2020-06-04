#ifndef __SHARED_CONTAINER__
#define __SHARED_CONTAINER__

#include <mutex>
#include <condition_variable>

#define MAX_POOL_SIZE 10

struct Node
{
	Node *next;
	unsigned char *buffer;
};

class SharedContainer
{
public:
	SharedContainer();
	void push(Node *node);
	Node* pop();
private:
	Node *head, *tail;
	std::mutex rw_mtx;
	std::condition_variable cv;
};

class SharedPool
{
public:
	SharedPool();
	~SharedPool();
	void push(Node *node);
	Node* pop();
private:
	Node *head;
	int count;
	std::mutex rw_mtx;
	std::condition_variable cv;
};

#endif
