#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <Windows.h>
#include <queue>
using namespace std;

// Event를 활용한 Lock 대기 응용.
// 내가 직접 Lock을 기다리지 않고 직원한테 부탁(갑질 메타)
// 여기서 직원은 커널에 존재한다.
// Spin Lock은 Lock이 풀릴때까지 계속 대기 하지만, Event를 활용하면 기다리는 자가 제 자리로 돌아간 이후에 커널에 존재하는 중재자가 깨워주는 개념. 중재자는 이벤트를 관찰하고 있음.

mutex m;
queue<int32_t> q;
HANDLE handle;

void Producer() 
{
	while (true)
	{
		{
			unique_lock<mutex> lock(m);
			q.push(100);
		}

		// 커널 이벤트 오브젝트에 Signal(TRUE) 상태로 바꿈.
		::SetEvent(handle);
		this_thread::sleep_for(1000ms);
	}
}

void Consumer() 
{
	while (true)
	{
		// 여기서 이벤트 오브젝트의 Signal값을 확인하고, TRUE이면 대기(wait) 상태로 들어감.
		::WaitForSingleObject(handle, INFINITE);
		// 이벤트객체 생성할때 파라미터로 bManualReset = FALSE를 줬기때문에 
		// 자동으로 함수가 끝나면 Signal이 Non-Signal(FALSE)로 바뀐다.
		// 수동으로 이벤트를 만들었을 경우 ::ResetEvent(handle) 사용하여 수동으로 바꿔줘야 함.
		// 이벤트 사용하지 않는 코드와 디버거 실행하여 CPU 점유율 비교해보기!

		unique_lock<mutex> lock(m);
		if (q.empty() == false) 
		{
			int32_t data = q.front();
			q.pop();
			cout << data << endl;
		}
	}
}

int main()
{
	// 커널 오브젝트
	// Usage Count : 몇 명이 사용하고 있는지.
	// Signal(파란불) / Non-Sigmal(빨간불) << bool
	// Auto / Manual << bool : 자동인지 수동인지.

	// 커널에 존재하는 이벤트(커널 오브젝트)
	// 리턴값으로 받는 HANDLE로 접근한다.
	handle = ::CreateEvent(NULL/*보안속성*/, FALSE/*수동(TRUE)? 자동(FALSE)?*/, FALSE /*초기상태 -> 빨간불*/, NULL);

	thread tr1(Producer);
	thread tr2(Consumer);

	tr1.join();
	tr2.join();

	::CloseHandle(handle);
}


#endif