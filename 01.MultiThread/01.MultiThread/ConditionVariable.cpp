//#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <Windows.h>
#include <queue>
using namespace std;


mutex m;
queue<int32_t> q;
HANDLE handle;

// 표준 mutex와 짝지어서 사용 됨.
// Event와 비슷하게 동작하게 됨.
// condition_variable은 User_Level Object(Not Kernel Object) 
condition_variable cv;

void Producer()
{
	while (true)
	{
		// 1) Lock을 잡고
		// 2) 공유 변수 값을 수정
		// 3) Lock을 풀고
		// 4) 조건변수 통해다른 쓰레드에게 통지

		{
			// 1) Lock을 잡고
			unique_lock<mutex> lock(m);
			// 2) 공유 변수 값을 수정
			q.push(100);
			// 3) Lock을 풀고
		}

		// 4) 조건변수 통해다른 쓰레드에게 통지
		// ::SetEvent(handle);
		cv.notify_one(); // wait중인 Thread가 있으면 딱 1개를 깨운다.
	}
}

void Consumer()
{
	while (true)
	{
		//::WaitForSingleObject(handle, INFINITE);
		unique_lock<mutex> lock(m);
		cv.wait(lock, []() {  /*대기 탈출 조건*/ return q.empty() == false; });
		// 1) Lock을 잡고
		// 2) 조건 확인
		// - 만족O => 빠져 나와서 이어서 코드를 진행.
		// - 만족X => Lock을 풀어주고 대기 상태로 진입.
		// 이전 Event 객체와 차이점은 Event 객체는 무저껀 대기 했지만, Condition Variable은 조건부 대기 상태로 빠짐.

		{
			int32_t data = q.front();
			q.pop();
			cout << q.size() << endl;
		}
	}
}

int main()
{
	thread tr1(Producer);
	thread tr2(Consumer);

	tr1.join();
	tr2.join();
}


#endif