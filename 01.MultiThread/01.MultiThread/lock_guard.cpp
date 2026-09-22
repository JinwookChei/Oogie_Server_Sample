//#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

std::vector<int> v;
// Mutual Exclusive(상호배타적)
std::mutex m;

// std::lock_guard 라는 것 이미 있음.
// RAII 패턴
template<typename T>
class LockGuard {
public:
	LockGuard(T& m) {
		_mutex = &m;
		_mutex->lock();
	}
	~LockGuard() {
		_mutex->unlock();
	}
private:
	T* _mutex;
};

// 주의 ! lock()을 수동적으로 사용하면 나중에 unlock()을 까먹거나 누락 할 수 있음. -> RAII 패턴을 이용한 lock_guard를 이용.
void Push() {
	for (int i = 0; i < 10000; ++i) {
		//m.lock();	// 락

		LockGuard<std::mutex> lockGuard(m);
		//std::lock_guard<std::mutex> lockGuard(m);
		//std::unique_lock<std::mutex> lockGuard(m, std::defer_lock);
		//lockGuard.lock();

		v.push_back(i);

		//m.unlock(); // 언락
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