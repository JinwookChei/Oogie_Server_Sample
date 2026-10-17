#include "pch.h"
#include "PlayerManager.h"
#include "AccountManager.h"


AccountManager GAccountManager;

void AccountManager::AccountThenPlayer()
{
	WRITE_LOCK;				// 1) Account 락 획득 (함수가 끝날 때까지 유지)
	GPlayerManager.Lock();	// 2) Account를 쥔 채로 Player 락 획득 → 간선 Account -> Player
}

void AccountManager::Lock()
{
	WRITE_LOCK;	// 잡자마자 스코프를 벗어나며 WriteLockGuard 소멸자가 해제
}