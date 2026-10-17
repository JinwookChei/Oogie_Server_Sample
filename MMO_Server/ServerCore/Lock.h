#pragma once
#include "Types.h"


/*
	[Lock] Reader-Writer SpinLock

	- 무엇을 하는가
		읽기(Read)는 여러 스레드가 동시에, 쓰기(Write)는 한 스레드만 독점하도록 해주는 락.
		표준 mutex는 읽기/쓰기 구분 없이 무조건 한 스레드만 통과시키기 때문에 직접 만든다.

	- 왜 필요한가 (예: 퀘스트 보상 목록 조회 GetRewardList(questId))
		보상 목록은 거의 항상 "읽기"만 하고, "수정"은 운영 중 드물게(0.00001% 수준) 일어난다.
		· mutex로 매번 잠그면 → 읽기끼리도 줄을 서서 기다린다. (읽기끼리는 동시에 해도 안전한데 성능 낭비)
		· 아예 안 잠그면     → 드물게 수정이 일어날 때 데이터가 꼬인다.
		→ "평소엔 다 같이 읽고, 수정할 때만 잠깐 독점" 하는 Reader-Writer Lock이 해답.
		→ 읽기는 매우 많고 쓰기는 매우 적은 데이터에 적합하다.

	- 동작 원리: 32bit 하나(_lockFlag)에 상태를 모두 담는다.
		[WWWWWWWW][WWWWWWWW][RRRRRRRR][RRRRRRRR]
		· 상위 16bit (W) : Write Lock을 소유한 스레드 ID (0이면 소유자 없음)
		· 하위 16bit (R) : 현재 Read Lock을 잡고 있는 횟수 (공유 카운트)
		→ 값 하나로 관리하므로 CAS(compare_exchange) 한 번에 상태를 원자적으로 바꿀 수 있다.
		→ SpinLock: 커널 대기 없이 루프를 돌며 재시도하고, 일정 횟수마다 yield로 양보한다.

	- 락 순서 규칙 (같은 스레드 기준)
		· W -> W (o) : Write 소유 중 다시 Write  → 재귀 허용 (_writeCount로 횟수 관리)
		· W -> R (o) : Write 소유 중 Read       → 허용 (이미 독점 중이므로 안전)
		· R -> W (x) : Read 소유 중 Write       → 불가 (다른 Reader가 있을 수 있어 교착 위험)
*/
class Lock
{
	/*
		[상수]
		- ACQUIRE_TIMEOUT_TICK : 락 획득 대기 한계 (ms). 넘으면 데드락으로 보고 CRASH.
		- MAX_SPIN_COUNT       : yield 하기 전까지 연속으로 시도하는 횟수.
		- WRITE_THREAD_MASK    : _lockFlag에서 상위 16bit(W, 소유 스레드 ID)만 뽑는 마스크.
		- READ_COUNT_MASK      : _lockFlag에서 하위 16bit(R, 읽기 카운트)만 뽑는 마스크.
		- EMPTY_FLAG           : 아무도 Read/Write 하지 않는 상태.
	*/
	enum : uint32
	{
		ACQUIRE_TIMEOUT_TICK = 10000,
		MAX_SPIN_COUNT = 5000,
		WRITE_THREAD_MASK = 0xFFFF0000,
		READ_COUNT_MASK = 0x0000FFFF,
		EMPTY_FLAG = 0x00000000
	};

public:
	/* 쓰기(독점) 락 획득. 같은 스레드의 재귀 획득 허용. */
	void WriteLock();
	/* 쓰기 락 해제. 재귀 횟수가 0이 될 때 실제로 풀린다. */
	void WriteUnlock();
	/* 읽기(공유) 락 획득. Writer가 없으면 여러 스레드가 동시에 획득 가능. */
	void ReadLock();
	/* 읽기 락 해제. 읽기 카운트를 1 감소. */
	void ReadUnLock();

private:
	Atomic<uint32> _lockFlag = EMPTY_FLAG;	// [W 16bit][R 16bit] 상태값
	uint16 _writeCount = 0;					// Write 재귀 횟수. 소유 스레드만 건드리므로 atomic 불필요

};

/*
	[ReadLockGuard] - RAII 패턴
	- 생성자에서 ReadLock, 소멸자에서 ReadUnLock을 호출한다.
	- 스코프를 벗어나면(return, 예외 포함) 자동으로 풀리므로 Unlock 누락을 막아준다.
	- 보통 직접 쓰지 않고 CoreMacro.h의 READ_LOCK 매크로로 사용한다.
*/
class ReadLockGuard
{
public:
	ReadLockGuard(Lock& lock) : _lock(lock) 
	{
		_lock.ReadLock();  
	}
	~ReadLockGuard() 
	{
		_lock.ReadUnLock();
	}

private:
	Lock& _lock;
};


/*
	[WriteLockGuard] - RAII 패턴
	- 생성자에서 WriteLock, 소멸자에서 WriteUnlock을 호출한다.
	- 보통 직접 쓰지 않고 CoreMacro.h의 WRITE_LOCK 매크로로 사용한다.
*/
class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock) : _lock(lock)
	{
		_lock.WriteLock();
	}
	~WriteLockGuard()
	{
		_lock.WriteUnlock();
	}

private:
	Lock& _lock;
};