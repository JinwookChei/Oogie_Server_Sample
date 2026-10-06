//#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
using namespace std::chrono_literals;

// Sleep을 사용하면 락이 잡힌 공간에 집입할때 락이 잠겨있으면 일딴 다시 제 자리로 돌아 간 이후, 랜덤 시간이 지난 후 다시 확인한다.
// 운빨 메타
// SpinLock.cpp 코드에 std::this_thread::sleep_for(std::chrono::milliseconds(100)); 한 줄만 추가하면됨.

class SpinLock
{
public:
	void lock() 
	{
		bool expected = false;
		bool desired = true;

		while (_locked.compare_exchange_strong(expected, desired) == false) 
		{
			expected = false;

			// 1. 스레드 상태의 변화 (작업 관리)
			// 대기 중(Blocked / Sleeping) : sleep_for(100ms)가 호출되면 해당 스레드는 CPU 점유를 중단하고 OS에 의해 대기 상태로 전환됩니다.이때 CPU 자원을 전혀 쓰지 않습니다.
			// 시간 완료(Ready) : 100밀리초가 지나면 OS 스케줄러가 이 스레드를 다시 "실행 가능한 상태(Ready)"로 바꿉니다.
			// 실행 재개(Running) : OS가 CPU를 할당해 주는 순간 다음 라인의 코드부터 실행됩니다.
			std::this_thread::sleep_for(100ms);


			//std::this_thread::sleep_for(std::chrono::milliseconds(100));
			// std::this_thread::sleep_for(0ms) == std::this_thread::yield();
			//std::this_thread::sleep_for(0ms);
			//std::this_thread::yield();
		}
		
	}
	void unlock()
	{
		_locked.store(false);
	}

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