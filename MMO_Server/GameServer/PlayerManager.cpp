#include "pch.h"
#include "PlayerManager.h"
#include "AccountManager.h"


PlayerManager GPlayerManager;

void PlayerManager::PlayerThenAccount()
{
	WRITE_LOCK;				// 1) Player 락 획득 (함수가 끝날 때까지 유지)
	GAccountManager.Lock();	// 2) Player를 쥔 채로 Account 락 획득 → 간선 Player -> Account
}

void PlayerManager::Lock()
{
	WRITE_LOCK;	// 잡자마자 스코프를 벗어나며 WriteLockGuard 소멸자가 해제
}