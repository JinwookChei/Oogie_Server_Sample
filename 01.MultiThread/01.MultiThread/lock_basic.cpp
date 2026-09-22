//#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

std::vector<int> v;
// Mutual Exclusive(상호배타적)
std::mutex m;

// 주의 ! lock()을 수동적으로 사용하면 나중에 unlock()을 까먹거나 누락 할 수 있음. -> RAII 패턴을 이용한 lock_guard를 이용.
void Push() {
	for (int i = 0; i < 10000; ++i) {
		m.lock();	// 락
		v.push_back(i);
		m.unlock(); // 언락
	}
}

int main() {
	std::thread t1(Push);
	std::thread t2(Push);

	t1.join();
	t2.join();

	std::cout << v.size() << std::endl;

	return 0;
}

#endif