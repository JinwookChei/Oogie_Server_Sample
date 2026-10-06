//#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include <mutex>

// SpinLock은 계속기다려서 CPU자원을 낭비하는 단점(CPU 점유율 증가)은 존재하지만
// Context Switch 비용이 없다.
// 즉, 금방 Lock이 풀릴 것 같은 경우. SpinLock을 이용한다.


// 문제가 없어보이지만 원자성이 깨진 코드
//class SpinLock
//{
//public:
//	void lock()
//	{
//		while (_locked)
//		{
//
//		}
//		_locked = true;
//	}
//
//	void unlock()
//	{
//		_locked = false;
//	}
//private:
//	// volatile? 컴파일러한테 최적화 하지 말라고 명령.
//	volatile bool _locked = false;
//};
//
//int sum = 0;
//std::mutex m;
//SpinLock spinLock;
//
//void Add()
//{
//	for (int i = 0; i < 1000000; ++i)
//	{
//		std::lock_guard<SpinLock> guard(spinLock);
//		++sum;
//	}
//}
//
//void Sub()
//{
//	for (int i = 0; i < 1000000; ++i)
//	{
//		std::lock_guard<SpinLock> guard(spinLock);
//		--sum;
//	}
//}
//
//int main()
//{
//	std::thread tr1(Add);
//	std::thread tr2(Sub);
//
//	tr1.join();
//	tr2.join();
//
//	std::cout << sum << std::endl;
//}





// CAS (Compare-And-Swap) -> 함수 내부의 동작을 하나의 원자로 만듦.
// CAS 사용 하여 개선 한 코드
class SpinLock 
{
public:
	//void lock() 
	//{
	//	bool expected = false;
	//	bool desired = true;

	//	// CAS 의사코드 - compare_exchange_strong(bool& expected, bool desired)
	//	/*if (_locked == expected) 
	//	{
	//		expected = _locked;
	//		_locked = desired;
	//		return true;
	//	}
	//	else 
	//	{
	//		expected = _locked;
	//		return false;
	//	}*/

	//	while (_locked.compare_exchange_strong(expected, desired) == false) 
	//	{
	//		expected = false;
	//	}
	//	
	//}
	//void unlock()
	//{
	//	_locked.store(false);
	//}

private:
	std::atomic<bool> _locked = false;
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