#pragma once
#include <stack>
#include <map>
#include <vector>

/*
	[DeadLockProfiler] - 락 순서 그래프로 데드락을 "미리" 탐지하는 디버그용 도구

	- 무엇을 하는가
		락을 잡을 때마다 "A를 잡은 상태에서 B를 잡았다"는 순서(A -> B)를 기록해
		락들을 정점(Vertex), 잡는 순서를 간선(Edge)으로 하는 방향 그래프를 만든다.
		새 간선이 생길 때마다 DFS로 사이클을 검사하고, 사이클이 있으면 CRASH 한다.

	- 왜 사이클 = 데드락인가 (예: GameServer의 PlayerManager / AccountManager)
		· 스레드1 : Player 잠금 -> Account 잠금   (간선 Player -> Account)
		· 스레드2 : Account 잠금 -> Player 잠금   (간선 Account -> Player)
		→ Player -> Account -> Player 사이클.
		→ 두 스레드가 타이밍 좋게 하나씩 잡으면 서로 상대 락을 영원히 기다린다.
		→ 실제 데드락은 타이밍이 맞아야만 터지지만, 이 도구는 "순서가 엇갈렸다"는 사실만으로
		  잡아내므로 테스트 중 운 좋게 안 터진 데드락도 발견할 수 있다.

	- 사용법
		Lock.cpp의 Read/WriteLock, Unlock에서 _DEBUG일 때만 PushLock / PopLock을 호출한다.
		락 이름은 CoreMacro.h의 매크로가 typeid(this).name()(클래스 이름)으로 넘겨준다.
		→ 같은 클래스의 락은 하나의 정점으로 취급된다.

	- 비용
		내부 Mutex 하나로 모든 스레드가 직렬화되고, 새 간선마다 전체 DFS를 돌기 때문에 느리다.
		그래서 Release 빌드에서는 호출하지 않는다.
*/
class DeadLockProfiler
{
public:
	/* 락 획득 직전 호출. 락 ID를 발급하고, 직전에 잡은 락 -> 이 락 간선을 추가하며 사이클 검사. */
	void PushLock(const char* name);
	/* 락 해제 직전 호출. 잡은 순서의 역순으로 푸는지 확인하고 스택에서 뺀다. */
	void PopLock(const char* name);
	/* 지금까지 기록된 락 순서 그래프 전체에 대해 사이클(데드락 가능성)을 검사한다. */
	void CheckCycle();

private:
	/* cur 정점에서 시작하는 DFS. 역방향 간선(= 사이클)을 발견하면 경로를 출력하고 CRASH. */
	void Dfs(int32 cur);

private:
	/*
		[락 이름 <-> ID 매핑]
		- 그래프 연산은 정수 ID가 편하므로 이름을 0부터 순서대로 ID로 바꿔 쓴다.
		- key가 const char* 이므로 "문자열 내용"이 아니라 "포인터 주소"로 비교한다.
		  typeid(...).name()은 같은 타입이면 같은 주소를 돌려주기 때문에 동작한다.
	*/
	unordered_map<const char*, int32> _nameToId;
	unordered_map<int32, const char*> _idToName;	// 사이클 발견 시 경로 출력용

	/*
		[현재 잡고 있는 락 스택]
		- 락은 잡은 순서의 역순으로 풀어야 하므로 stack으로 관리한다.
		- top = 가장 최근에 잡은 락. 새 락을 잡을 때 top -> 새 락 간선을 만든다.
		- 주의: 현재는 멤버 변수 하나를 모든 스레드가 공유한다.
		  "잡고 있는 락"은 스레드마다 다르므로, 여러 스레드가 서로 다른 락을 잡으면
		  다른 스레드의 락이 top에 올라와 잘못된 간선이 생기거나 PopLock에서 INVALID_UNLOCK이 날 수 있다.
		  정확히 하려면 CoreTLS의 thread_local 스택(예: LLockStack)으로 옮겨야 한다.
	*/
	stack<int32> _lockStack;

	/*
		[락 순서 그래프 (인접 리스트)]
		- _lockHistory[A] = { B, C } : A를 잡은 상태에서 B, C를 잡은 적이 있다. (A -> B, A -> C)
		- 한 번 기록된 간선은 지우지 않는다. (과거에 한 번이라도 있었던 순서는 계속 위험 요소)
	*/
	map<int32, set<int32>> _lockHistory;

	Mutex _lock;	// 위 자료구조들을 여러 스레드가 동시에 건드리지 못하게 보호

private:
	/*
		[DFS용 임시 데이터] - CheckCycle 호출마다 초기화되고, 끝나면 비운다.
		간선 cur -> nxt 를 만났을 때 아래 정보로 간선 종류를 구분한다.
		· nxt 미발견                                  → 트리 간선 (그냥 방문)
		· order[cur] < order[nxt]                     → 순방향 간선 (nxt는 이미 끝난 후손, 사이클 아님)
		· order[cur] > order[nxt] && !finished[nxt]   → 역방향 간선 (nxt는 아직 탐색 중인 조상 = 사이클!)
		· order[cur] > order[nxt] &&  finished[nxt]   → 교차 간선 (다른 가지, 사이클 아님)
	*/
	vector<int32> _discoveredOrder;	// 노드가 발견된 순서를 기록하는 배열 (-1 = 아직 발견 안 됨)
	int32 _discoveredCount = 0;	// 지금까지 발견된 노드 수 (다음에 매길 발견 순서)
	vector<bool> _finished; // Dfs(i)가 종료되었는지 여부 (false인데 발견됨 = 현재 DFS 경로 위에 있음)
	vector<int32> _parent;	// DFS 트리에서의 부모. 사이클 경로를 거슬러 올라가 출력할 때 사용
};

