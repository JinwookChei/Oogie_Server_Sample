#pragma once

/*
	[PlayerManager] - DeadLockProfiler 테스트용 클래스
	- AccountManager와 서로의 락을 반대 순서로 잡도록 만들어 데드락 상황을 재현한다.
*/
class PlayerManager
{
	USE_LOCK;	// Lock _locks[1]; 선언

public:
	/* 내 락(Player)을 잡은 상태에서 AccountManager의 락을 잡는다. (Player -> Account) */
	void PlayerThenAccount();
	/* 내 락만 잡았다가 바로 푼다. 다른 매니저가 "내 락을 잡는" 상황을 만들기 위한 함수. */
	void Lock();
};

extern PlayerManager GPlayerManager;