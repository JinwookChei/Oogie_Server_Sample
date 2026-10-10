#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
using namespace std;

thread_local int32_t LThreadId = 0;

void ThreadMain(int32_t threadId) 
{
	LThreadId = threadId;
	while (true)
	{
		cout << "Hi I am Tread" << LThreadId << endl;
		this_thread::sleep_for(1s);
	}
}

int main()
{
	vector<thread>threads;
	for (int32_t i = 0; i < 10; ++i)
	{
		int32_t threadId = i + 1;
		threads.push_back(thread(ThreadMain, threadId));
	}
	
	for (thread& t : threads)
	{
		t.join();
	}
}

#endif