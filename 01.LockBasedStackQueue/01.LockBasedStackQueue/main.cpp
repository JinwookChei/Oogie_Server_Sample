#include <iostream>
#include <thread>
#include <chrono>
#include "ConcurrentQueue.h"
#include "ConcurrentStack.h"
#include "LockFreeStack.h"
#include "LockFreeStack2.h"
#include "LockFreeStack3.h"
using namespace std;

LockQueue<int32_t> q;
LockStack<int32_t> s;
LockFreeStack<int32_t> lockFreeStack;
LockFreeStack2<int32_t> lockFreeStack2;
LockFreeStack3<int32_t> lockFreeStack3;

void Push()
{
	while (true)
	{
		int32_t value = rand() % 100;
		lockFreeStack.Push(value);

		//std::this_thread::sleep_for(10ms);
	}
}

void Pop()
{
	while (true)
	{
		int32_t data = 0;
		if (lockFreeStack.TryPop(data))
		{
			//cout << data << endl;
		}
	}
}

int main()
{
	thread t1(Push);
	thread t2(Pop);
	thread t3(Pop);
	thread t4(Pop);
	thread t5(Pop);

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
}
