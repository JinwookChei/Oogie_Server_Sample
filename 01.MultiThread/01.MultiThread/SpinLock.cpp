#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include <mutex>

// SpinLock은 계속기다려서 CPU자원을 낭비하는 단점은 존재하지만
// Context Switch 비용이 없다.
// 즉, 금방 Lock이 풀릴 것 같은 경우. SpinLock을 이용한다.

class SpinLock 
{
public:
	void lock() 
	{

	}
	void unlock()
	{

	}
private:
	// volatile? 컴파일러한테 최적화 하지 말라고 명령.
	volatile bool _locked = false;
};

int sum = 0;
std::mutex m;
SpinLock spinLock;

void Add() 
{
	for (int i = 0; i < 1000000; ++i) 
	{
		std::lock_guard<SpinLock> guard(spinLock);
		++sum;
	}
}

void Sub()
{
	for (int i = 0; i < 1000000; ++i)
	{
		std::lock_guard<SpinLock> guard(spinLock);
		--sum;
	}
}

int main()
{
	std::thread tr1(Add);
	std::thread tr2(Sub); 

	tr1.join();
	tr2.join();

	std::cout << sum << std::endl;
}

#endif