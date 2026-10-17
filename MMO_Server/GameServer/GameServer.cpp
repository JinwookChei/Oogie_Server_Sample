#include "pch.h"
#include <iostream>
#include <windows.h>
#include <chrono>
#include "CorePch.h"
#include <thread>
#include "CoreMacro.h"
#include "ThreadManager.h"

#include "AccountManager.h"
#include "PlayerManager.h"


int main()
{
	// 이 코드는 DeadLockProfiler를 테스트 하기 위한 DeadLock을 구현한 코드입니다.
	// 두 스레드가 같은 두 락을 서로 반대 순서로 잡는다.
	//  · 스레드1 : Player -> Account
	//  · 스레드2 : Account -> Player
	// 실제 데드락은 두 스레드가 첫 번째 락을 동시에 잡아야만 발생하지만,
	// Debug 빌드에서는 DeadLockProfiler가 순서 그래프의 사이클(Player -> Account -> Player)을 보고
	// 실제로 멈추기 전에 "DEADLOCK_DETECTED"로 CRASH 시킨다.

	// 스레드1 : PlayerManager 락을 잡은 상태에서 AccountManager 락을 잡는다.
	GThreadManager->Launch([=]
		{
			while (true)
			{
				cout << "PlayerThenAccount" << endl;
				GPlayerManager.PlayerThenAccount();
				this_thread::sleep_for(100ms);
			}
		});

	// 스레드2 : AccountManager 락을 잡은 상태에서 PlayerManager 락을 잡는다. (스레드1과 반대 순서)
	GThreadManager->Launch([=]
		{
			while (true)
			{
				cout << "AccountThenPlayer" << endl;
				GAccountManager.AccountThenPlayer();
				this_thread::sleep_for(100ms);
			}
		});

	GThreadManager->Join();
}