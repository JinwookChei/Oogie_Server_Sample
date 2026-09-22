#include "pch.h"
#include <iostream>
#include "CorePch.h"

// #include <Windows.h> -> -> CreateThread();
// 라는 window 종속적인 쓰레드를 사용해 왔으나, C++ 11부터 
// 운영체제 독립적인 #include <thread>를 사용 할 수 있게 되었음.
// 
#include <thread>

void HelloThread() {
	cout << "HelloTherad" << endl;
}

int main()
{
	std::thread t(HelloThread);
	
	cout << "Hello Main" << endl;

	t.join();
}