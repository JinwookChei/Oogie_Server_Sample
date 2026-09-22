//#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include <atomic>

// 주의! 아토믹 객체의 연산은 매우 느리다. 꼭 필요한 경우에만 사용 할 것.
std::atomic<int> sum = 0;


void Add() {
	for (int i = 0; i < 1000000; ++i) {
		++sum;
		sum.fetch_add(1); // ++sum과 같은 코드.
	}
}

void Sub() {
	for (int i = 0; i < 1000000; ++i) {
		--sum;
		sum.fetch_add(-1); // --sum과 같은 코드.
	}
}

int main() {
	std::thread t1(Add);
	std::thread t2(Sub);

	t1.join();
	t2.join();


	std::cout << sum << std::endl;
	return 0;
}

#endif