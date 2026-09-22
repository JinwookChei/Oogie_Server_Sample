#include <iostream>
#include <vector>
using namespace std;

//#include <Windows.h>
//"Windows.h"에는 윈도우 전용 Thread생성 함수인 CreateThread()가 존재하나, 
//C++11부터 OS의 독립적인 thread라이브러리가 추가되었음.

#include <thread>

void HelloWorld() {
	cout << "HelloThread!" << endl;
}

void HelloWorld_Params(int num) {
	cout << "HelloThread_Params-" << num << "!" << endl;
}

int main() {
	// unsigned int coreCnt = tr1.hardware_concurrency(); 
	// 나의 CPU 코어 갯수 


	// auto id = tr1.get_id(); 
	// Thread ID를 불러옴.(Thread 끼리 ID는 Unique 함.)


	// tr1.detach(); 
	// join()과 반대로 쓰레드의 연결고리를 끊는 기능. 끊긴 thread는 백그라운드에서 알아서 도는 독립적인 Thread로 변함. 
	// 즉, std::thread에서 분리하여 백그라운드에서 도는 thread로 변함.


	// tr1.joinable(); 
	// 연결된 thread가 있는지 없는지 판단하여 bool 반환.(detach()된 thread는 false 반환)

	// ------------------- 01 ----------------------
	std::thread tr1;
	auto id1 = tr1.get_id();

	tr1 = std::thread(HelloWorld);
	auto id2 = tr1.get_id();

	if (tr1.joinable()) {
		tr1.join();
	}
	// ---------------------------------------------


	// ------------------- 02 ----------------------
	std::thread tr2 = std::thread(HelloWorld_Params, 10);
	if (tr2.joinable()) {
		tr2.join();
	}
	// ---------------------------------------------


	// ------------------- 03 ----------------------
	std::vector<std::thread> v;
	for (int i = 1; i <= 10; ++i) {
		v.push_back(std::thread(HelloWorld_Params, i));
	}
	for (int i = 0; i < 10; ++i) {
		if (v[i].joinable()) {
			v[i].join();
		}
	}
	// ---------------------------------------------


	cout << "HelloMain!" << endl;
}