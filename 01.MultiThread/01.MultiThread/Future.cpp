//#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
using namespace std;


// 프로그램 동작중 굳이 한번만 실행되는 함수를 비동기로 실행할때 쓰레드를 만들고, 지우는 것보다 future로 만들어서 사용하는 것이 조금 더 간편하다.
int64_t Calculate()
{
	int64_t sum = 0;
	for (int32_t i = 0; i < 100000; ++i)
	{
		sum += i;
	}
	return sum;
}

void PromiseWorker(std::promise<string>&& promise)
{
	promise.set_value("Secret Message");
}

void TaskWorker(std::packaged_task<int64_t(void)>&& task)
{
	task();
}

int main()
{
	// future
	{
		// 1) std::launch::async								-> 별도의 쓰레드를 만들어서 실행하세요.
		// 2) std::launch::deferred								-> lazy evalution 지연해서 실행하세요. (future.get() 사용한 라인에서 실행 하게 됨)
		// 3) std::launch::deferred | std::launch::deferred		-> 둘 중 알아서 골라주세요.
		//std::future<int64_t> future = std::async(std::launch::deferred, Calculate);
		std::future<int64_t> future = std::async(std::launch::async, Calculate);
		
		// ... 다른일 중 ...

		// Future의 상태도 볼 수 있다.
		std::future_status status = future.wait_for(1ms);
		if (status == future_status::ready)
		{
			// 일감 완료.
		}

		int64_t sum = future.get();
	}


	// 멤버 함수도 호출 할 수 있다.
	{
		class Knight
		{
		public:
			int64_t GetHP() { return 100; }
		};

		Knight knight;
		std::future<int64_t> future = std::async(std::launch::async, &Knight::GetHP, knight);
	}

	// Promise
	{
		// 미래(std::future)에 결과물을 반환해줄꺼라 약속(std::promise) 해줘.(계약서 느낌)
		std::promise<string> promise;
		std::future<string> future = promise.get_future();

		std::thread t(PromiseWorker, std::move(promise));

		string message = future.get();
		cout << message << endl;

		t.join();
	}

	// std::packaged_task
	{
		std::packaged_task<int64_t(void)> task(Calculate);
		std::future<int64_t> future = task.get_future();

		std::thread t(TaskWorker, std::move(task));

		int64_t sum = future.get();
		cout << sum << endl;

		t.join();
	}
}

// 결론)
// mutex, condition_variable까지 만들지 않고 일회성인 함수인 애들은 이런식으로 처리 할 수도 있다.
// 특히나 한 번 발생하는 이벤트에 유용하다!
// 1) async
// 원하는 함수를 비동기적으로 실행
// 2) promise
// 결과물을 promise를 통해 future로 받아줌
// 3) packaged_task
// 원하는 함수의 실행 결괄르 packaged_task를 통해 future로 받아 줌.

#endif