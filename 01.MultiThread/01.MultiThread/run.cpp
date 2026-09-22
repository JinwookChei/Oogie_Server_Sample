#define RUN // 실행하고 싶을 때 주석 해제
#ifdef RUN

#include <iostream>
#include <thread>
#include "UserManager.h"
#include "AccountManager.h"


void Func1() 
{
	for (int i = 0; i < 100; ++i) {
		UserManager::instance()->ProcessSave();
	}
}

void Func2()
{
	for (int i = 0; i < 100; ++i) {
		AccountManager::instance()->ProcessLogin();
	}
}

int main() 
{
	// DeadLock 발생!
	std::thread tr1(Func1);
	std::thread tr2(Func2);

	tr1.join();
	tr2.join();

	std::cout << "Done" << std::endl;
}

#endif