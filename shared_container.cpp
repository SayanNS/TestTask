#include "shared_container.h"

SharedContainer::SharedContainer()
{
	head = NULL;
	tail = (Node*) -1; // NOT NULL VALUE
}

void SharedContainer::push(Node *node)
{
	rw_mtx.lock();

	if (node != NULL)
	{
		node->next = NULL;
	}

	if (head == NULL)
	{
		head = node;
		tail = node;
		
		rw_mtx.unlock();
		cv.notify_one();
	}
	else
	{
		tail->next = node;
		tail = node;

		rw_mtx.unlock();
	}
}

Node* SharedContainer::pop()
{
	std::unique_lock<std::mutex> lk(rw_mtx);

	if (head == NULL)
	{
		if (tail == NULL)
		{
			lk.unlock();
			return NULL;
		}

		cv.wait(lk);

		if (tail == NULL)
		{
			lk.unlock();
			return NULL;
		}
	}

	Node* node = head;
	head = node->next;
	lk.unlock();

	return node;
}

SharedPool::SharedPool()
{
	head = NULL;
	count = 0;
}

SharedPool::~SharedPool()
{
	while (head)
	{
		Node *node = head;
		head = node->next;

		delete [] node->buffer;
		delete node;
	}
}

void SharedPool::push(Node* node)
{
	rw_mtx.lock();

	if (head == NULL && count == MAX_POOL_SIZE)
	{
		node->next = head;
		head = node;

		rw_mtx.unlock();
		cv.notify_one();
	}
	else
	{
		node->next = head;
		head = node;

		rw_mtx.unlock();
	}
}

Node* SharedPool::pop()
{
	std::unique_lock<std::mutex> lk(rw_mtx);

	if (head == NULL)
	{
		if (count < MAX_POOL_SIZE)
		{
			count++;
			lk.unlock();

			return NULL;
		}
		else
		{
			cv.wait(lk);
		}
	}

	Node* node = head;
	head = node->next;
	lk.unlock();

	return node;
}
