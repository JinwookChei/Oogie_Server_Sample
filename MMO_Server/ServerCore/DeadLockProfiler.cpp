#include "pch.h"
#include "DeadLockProfiler.h"

/*
	[DeadLockProfiler::PushLock]
	- 동작 순서
		1) 락 이름으로 ID를 찾고, 처음 보는 이름이면 새 ID(0, 1, 2 ...)를 발급한다.
		2) 이미 잡고 있는 락이 있다면 "직전 락(prevId) -> 이번 락(lockId)" 간선을 확인한다.
		   · 같은 락을 다시 잡는 경우(재귀 락)는 간선을 만들지 않는다. (자기 자신으로의 사이클 방지)
		   · 처음 생긴 간선일 때만 그래프에 추가하고 CheckCycle()로 사이클을 검사한다.
		     (이미 있던 간선이면 그래프 모양이 그대로이므로 다시 검사할 필요가 없다)
		3) 이번 락을 스택에 쌓는다.
*/
void DeadLockProfiler::PushLock(const char* name)
{
	LockGuard guard(_lock);

	// 아이디를 찾거나 발급한다.
	int32 lockId = 0;

	auto findIter = _nameToId.find(name);
	if (findIter == _nameToId.end())
	{
		lockId = static_cast<int32>(_nameToId.size());	// 현재 등록된 개수 = 다음 ID
		_nameToId[name] = lockId;
		_idToName[lockId] = name;
	}
	else
	{
		lockId = findIter->second;
	}

	// 잡고 있는 락이 있었다면
	if (_lockStack.empty() == false)
	{
		// 기존에 발견되지 않은 케이스라면 데드락 여부 다시 확인한다.
		const int32 prevId = _lockStack.top();
		if (lockId != prevId)
		{
			set<int32>& history = _lockHistory[prevId];
			if (history.find(lockId) == history.end())
			{
				history.insert(lockId);	// 새 간선 prevId -> lockId
				CheckCycle();
			}
		}
	}

	_lockStack.push(lockId);
}

/*
	[DeadLockProfiler::PopLock]
	- 잡은 락이 없는데 푼다           → MULTIPLE_UNLOCK (중복 해제)
	- 가장 최근에 잡은 락이 아닌 걸 푼다 → INVALID_UNLOCK (잡은 역순으로 풀지 않음)
	- 정상이라면 스택에서 꺼낸다. (그래프의 간선은 지우지 않는다)
*/
void DeadLockProfiler::PopLock(const char* name)
{
	LockGuard guard(_lock);

	if (_lockStack.empty())
		CRASH("MULTIPLE_UNLOCK");

	int32 lockId = _nameToId[name];
	if (_lockStack.top() != lockId)
		CRASH("INVALID_UNLOCK");

	_lockStack.pop();
}

/*
	[DeadLockProfiler::CheckCycle]
	- DFS용 임시 배열을 락 개수만큼 초기화한 뒤, 모든 정점에서 Dfs를 시작한다.
	  (그래프가 여러 덩어리로 나뉘어 있을 수 있으므로 0번부터 끝까지 다 시도.
	   이미 방문한 정점은 Dfs 안에서 바로 return 된다)
	- 사이클이 있으면 Dfs 내부에서 CRASH 하므로, 여기까지 돌아오면 사이클이 없는 것.
	- 검사가 끝나면 임시 배열은 메모리를 위해 비운다.
*/
void DeadLockProfiler::CheckCycle()
{
	const int32 lockCount = static_cast<int32>(_nameToId.size());
	_discoveredOrder = vector<int32>(lockCount, -1);
	_discoveredCount = 0;
	_finished = vector<bool>(lockCount, false);
	_parent = vector<int32>(lockCount, -1);

	for (int32 lockId = 0; lockId < lockCount; ++lockId)
		Dfs(lockId);

	_discoveredOrder.clear();
	_finished.clear();
	_parent.clear();
}

/*
	[DeadLockProfiler::Dfs]
	- 방향 그래프의 사이클 판별: DFS 중 "역방향 간선"(아직 탐색이 끝나지 않은 조상으로 가는 간선)이
	  하나라도 있으면 사이클이다.
	- 동작 순서
		1) 이미 발견된 정점이면 종료. 아니면 발견 순서를 매긴다.
		2) cur에서 나가는 간선(cur을 잡은 뒤 잡은 락들)을 하나씩 본다.
			· 처음 보는 정점     → 부모를 기록하고 재귀 방문
			· 나보다 늦게 발견됨 → 순방향 간선, 무시
			· 나보다 먼저 발견 + 아직 종료 안 됨 → 역방향 간선 = 사이클 → 경로 출력 후 CRASH
			· 나보다 먼저 발견 + 이미 종료됨     → 교차 간선, 무시
		3) 모든 간선을 다 봤으면 cur의 탐색 종료를 표시한다.
	- 사이클 경로 출력 예 (Player=0, Account=1 일 때)
		Account -> Player       ← 사이클을 만든 역방향 간선 (cur -> nxt)
		Player -> Account       ← _parent를 따라 nxt까지 거슬러 올라가며 출력
*/
void DeadLockProfiler::Dfs(int32 cur)
{
	if (_discoveredOrder[cur] != -1)
		return;

	_discoveredOrder[cur] = _discoveredCount++;

	// 모든 인접한 정점을 순회한다.
	auto findIter = _lockHistory.find(cur);
	if (findIter == _lockHistory.end())
	{
		// cur을 잡은 뒤 다른 락을 잡은 적이 없다 → 나가는 간선 없음
		_finished[cur] = true;
		return;
	}

	set<int32>& nextSet = findIter->second;
	for (int32 nxt : nextSet)
	{
		// 아직 방문한 적이 없다면 방문한다.
		if (_discoveredOrder[nxt] == -1)
		{
			_parent[nxt] = cur;
			Dfs(nxt);
			continue;
		}

		// cur이 nxt보다 먼저 발견되었다면, nxt는 cur의 후손이다.
		// 순방향 간선.
		if (_discoveredOrder[cur] < _discoveredOrder[nxt])
			continue;

		// 순방향이 아니고, Dfs(nxt)가 아직 종료하지 않았다면, nxt는 cur의 선조이다.
		// 역방향 간선.
		// (nxt가 이미 종료되었다면 다른 가지의 교차 간선이므로 사이클이 아니다)
		if (_finished[nxt] == false)
		{
			printf("%s -> %s\n", _idToName[cur], _idToName[nxt]);

			// cur에서 부모를 따라 nxt까지 올라가며 사이클을 이루는 나머지 간선들을 출력
			int32 now = cur;
			while (true)
			{
				printf("%s -> %s\n", _idToName[_parent[now]], _idToName[now]);
				now = _parent[now];
				if (now == nxt)
					break;
			}

			CRASH("DEADLOCK_DETECTED");
		}
	}

	_finished[cur] = true;
}
