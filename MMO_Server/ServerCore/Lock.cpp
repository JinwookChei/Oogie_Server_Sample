#include "pch.h"
#include "Lock.h"
#include "CoreTLS.h"

/*
	[Lock::WriteLock]
	- 아무도 소유(Write)하거나 공유(Read)하고 있지 않을 때(_lockFlag == EMPTY_FLAG),
	  경합해서 소유권을 얻는다.
	- 동작 순서
		1) 이미 내가 Write 소유자라면 → 재귀 획득으로 즉시 성공.
		2) 아니라면 CAS로 "EMPTY_FLAG → [내 스레드ID][0]" 교체를 시도.
		   · CAS(compare_exchange): "값이 expected와 같을 때만 desired로 바꾼다"를 원자적으로 수행.
		   · Reader가 1명이라도 있거나 다른 Writer가 있으면 EMPTY가 아니므로 실패 → 재시도.
		3) MAX_SPIN_COUNT번 실패하면 yield로 CPU를 양보하고 다시 스핀.
		4) ACQUIRE_TIMEOUT_TICK을 넘기면 데드락으로 판단하고 CRASH.
	- 주의: LThreadId는 ThreadManager에서 1부터 발급된다.
	        ID가 0인(발급받지 않은) 스레드는 소유자 없음(0)과 구분되지 않으므로 사용하면 안 된다.
*/
void Lock::WriteLock()
{
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;	// 상위 16bit = 현재 Write 소유 스레드 ID
	if (LThreadId == lockThreadId)
	{
		_writeCount++;
		return;
	}

	const int64 beginTick = ::GetTickCount64();
	const uint32 desired = ((LThreadId << 16) & WRITE_THREAD_MASK);	// [내 스레드ID][R=0]
	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			uint32 expected = EMPTY_FLAG;	// CAS 실패 시 현재값으로 덮어써지므로 매번 다시 초기화
			if (_lockFlag.compare_exchange_strong(OUT expected, desired))
			{
				++_writeCount;
				return;
			}
		}

		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("LOCK_TIMEOUT");

		this_thread::yield();
	}
}

/*
	[Lock::WriteUnlock]
	- 재귀 횟수(_writeCount)를 1 줄이고, 0이 되면 _lockFlag를 EMPTY로 만들어 락을 완전히 푼다.
	- W -> R 순서로 잡았다면 Read를 먼저 다 풀어야 한다. (R 카운트가 남아 있으면 CRASH)
*/
void Lock::WriteUnlock()
{
	if ((_lockFlag.load() & READ_COUNT_MASK) != 0)
		CRASH("INVALID_UNLOCK_ORDER");

	const int32 lockCount = --_writeCount;
	if (lockCount == 0)
	{
		_lockFlag.store(EMPTY_FLAG);
	}
}

/*
	[Lock::ReadLock]
	- Writer가 없을 때(W 영역 == 0), 경합해서 공유 카운트(R)를 1 올린다.
	- 핵심 트릭: expected를 "현재값 & READ_COUNT_MASK"(W 영역을 0으로 지운 값)로 잡는다.
		· Writer가 없으면  → 실제값 == expected → CAS 성공, R + 1
		· Writer가 있으면  → 실제값엔 W bit가 있어 expected와 다름 → CAS 실패 → 재시도
		· 그 사이 다른 Reader가 R을 바꿔도 → CAS 실패 → 최신값으로 재시도
	- 스핀/yield/타임아웃 처리는 WriteLock과 동일.
	- 같은 스레드가 이미 Write 소유자라면(W -> R) 경합 없이 R 카운트만 올리고 즉시 성공.
	  (이미 독점 중이라 다른 스레드가 끼어들 수 없으므로 CAS 없이 fetch_add로 충분)
*/
void Lock::ReadLock()
{
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		_lockFlag.fetch_add(1);
		return;
	}

	const int64 beginTick = ::GetTickCount64();
	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			uint32 expected = (_lockFlag.load() & READ_COUNT_MASK);	// W 영역을 0으로 가정한 기대값
			if (_lockFlag.compare_exchange_strong(OUT expected, expected + 1))
				return;
		}

		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("LOCK_TIMEOUT");

		this_thread::yield();
	}
}

/*
	[Lock::ReadUnLock]
	- 공유 카운트(R)를 1 내린다.
	- fetch_sub는 "빼기 전 값"을 반환한다. 그 값의 R이 이미 0이었다면
	  잡지도 않은 Read Lock을 풀려는 것(중복 해제)이므로 CRASH.
*/
void Lock::ReadUnLock()
{
	if ((_lockFlag.fetch_sub(1) & READ_COUNT_MASK) == 0)
		CRASH("MULTIPLE_UNLOCK");
}
