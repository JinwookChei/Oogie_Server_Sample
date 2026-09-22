//#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>

int sum = 0;

void Add() {
	for (int i = 0; i < 1000000; ++i) {
		++sum;
	}
}

void Sub() {
	for (int i = 0; i < 1000000; ++i) {
		--sum;
	}
}

int main() {
	std::thread t1(Add);
	std::thread t2(Sub);

	t1.join();
	t2.join();

	// sum 결과가 0이 아닌 이유? 
	// disasembly코드로 보면 sum 함수는 
	// eax(레지스터) = sum;
	// eax = eax + 1;
	// sum = eax(레지스터)
	// 이렇게 진행되는데, 레지스터에서 계산중 일때 thread2이 계산되면 덮어쓰기가 되어 0이 아닌 결과가 나오게 된다.


	std::cout << sum << std::endl;
	return 0;
}

#endif