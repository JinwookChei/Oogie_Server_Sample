#pragma once
#include <stack>
#include <atomic>
#include <mutex>
#include <condition_variable>
using namespace std;

/*
 =====================================================================================
 [Lock-Free Stack2 - Pop 중인 스레드 수를 세서 삭제 시점을 늦추는 방식]

 ■ LockFreeStack1의 문제
	- TryPop에서 노드를 꺼내자마자 delete 했기 때문에,
	  같은 노드를 아직 읽고 있던 다른 스레드가 해제된 메모리에 접근할 수 있었습니다.

 ■ 해결 아이디어
	- 기존 노드의 내용을 읽는 곳은 TryPop 안뿐입니다.
	  (Push는 새 노드만 만들고, 기존 노드의 내용은 읽지 않음)
	- 그러므로 "지금 TryPop 안에 있는 스레드가 나 하나뿐"이라면,
	  내가 꺼낸 노드를 다른 스레드가 보고 있을 가능성이 없으므로 바로 삭제해도 안전합니다.
	- 다른 스레드가 TryPop 안에 있다면 바로 삭제하지 않고 _pendingList(삭제 대기 목록)에 넣어둡니다.
	- 나중에 TryPop 안에 혼자 있게 된 스레드가 대기 목록을 한 번에 정리합니다.

 ■ 사용하는 변수
	- _popCount    : 현재 TryPop을 실행 중인 스레드 수
	- _pendingList : 삭제를 미뤄둔 노드들의 연결 리스트 (첫 번째 노드를 가리킴)

 ■ ABA와의 관계
	- 어떤 스레드가 TryPop 안에서 노드를 보고 있는 동안에는 그 노드가 삭제되지 않으므로,
	  같은 주소가 재할당될 수 없습니다. 따라서 이 방식에서는 ABA도 함께 막힙니다.

 ■ 한계
	- TryPop이 계속 겹쳐서 호출되면 "혼자인 순간"이 오지 않아
	  _pendingList가 계속 길어지고 메모리가 해제되지 않습니다.

 ■ 이전 버전 대비 수정 사항
	- [수정 1] TryPop 시작 시 ++_popCount 추가
	- [수정 2] 대기 목록을 되돌려 놓을 때 nullptr 확인 추가
	- [수정 3] _pendingList를 nullptr로 초기화
 =====================================================================================
*/
template<typename T>
class LockFreeStack2
{
	struct Node
	{
		Node(const T& value) : data(value), next(nullptr) {}

		T data;
		Node* next;
	};

public:
	void Push(const T& value)
	{
		// 1) 새 노드 생성. 아직 스택에 연결되지 않았으므로 다른 스레드는 이 노드를 볼 수 없음
		Node* node = new Node(value);

		// 2) 새 노드의 next가 현재 head를 가리키도록 설정
		node->next = _head.load();

		// 3) _head가 아직 node->next와 같다면 _head를 node로 교체
		//    그 사이 다른 스레드가 _head를 바꿨다면 실패하고 다시 시도
		while (!_head.compare_exchange_weak(node->next, node))
		{
			// 실패하면 node->next에 최신 _head 값이 자동으로 들어감 -> 다시 시도
			// (compare_exchange_weak는 값이 같아도 가끔 실패할 수 있으므로 반복문 안에서 사용)
		}
	}

	bool TryPop(T& value)
	{
		// [수정 1] TryPop에 들어왔음을 먼저 알림
		// 반드시 _head를 읽기 전에 증가시켜야 함
		// _head를 읽은 뒤에 증가시키면, 그 사이에 다른 스레드가 "혼자다"라고 판단하고
		// 내가 읽은 노드를 삭제할 수 있음
		++_popCount;

		// 1) 현재 head를 읽음
		Node* oldHead = _head.load();

		// 2) oldHead가 nullptr이 아니고, _head가 아직 oldHead라면 _head를 oldHead->next로 교체
		//    _popCount에 내가 포함되어 있으므로, 내가 TryPop 안에 있는 동안 oldHead는 삭제되지 않음
		//    따라서 oldHead->next 읽기가 안전함
		while (oldHead && !_head.compare_exchange_weak(oldHead, oldHead->next))
		{
			// 실패하면 oldHead에 최신 _head 값이 자동으로 들어감 -> 다시 시도
		}

		// 3) 스택이 비어 있음 -> 꺼낸 노드가 없으므로 카운트만 줄이고 종료
		if (oldHead == nullptr)
		{
			--_popCount;
			return false;
		}

		// 4) oldHead는 _head에서 분리되었으므로, 새로 들어오는 스레드는 이 노드에 도달할 수 없음
		//    다만 이전부터 TryPop 안에 있던 스레드는 아직 이 노드를 보고 있을 수 있음
		value = oldHead->data;

		// 5) 바로 delete 하지 않고, 안전한 경우에만 삭제 (카운트 감소도 TryDelete 안에서 처리)
		TryDelete(oldHead);
		return true;
	}

	void TryDelete(Node* oldHead)
	{
		if (_popCount == 1)
		{
			// TryPop 안에 나 혼자만 있음

			// 1) 삭제 대기 목록을 통째로 가져오고, _pendingList는 비워둠
			//    혼자인 김에 다른 스레드가 미뤄둔 노드들도 함께 정리하려는 것
			Node* node = _pendingList.exchange(nullptr);

			// 2) 내 카운트를 빼면서, 목록을 가져오는 사이에 다른 스레드가 들어왔는지 확인
			//    (1번 확인과 exchange 사이에 다른 스레드가 들어와서
			//     자기가 보고 있는 노드를 대기 목록에 넣었을 수 있기 때문에 한 번 더 확인)
			if (--_popCount == 0)
			{
				// 아무도 들어오지 않았음 -> 가져온 목록을 삭제해도 안전
				// 이후에 새로 들어오는 스레드는 이 목록에 접근할 방법이 없음
				// (목록은 이미 _pendingList에서 분리되었고, _head에서도 도달할 수 없음)
				DeleteNodes(node);
			}
			else if (node) // [수정 2] 가져온 목록이 비어 있으면 되돌려 놓을 것이 없음
			{
				// 다른 스레드가 들어왔음
				// 그 스레드가 목록 안의 노드를 보고 있을 수 있으므로 삭제하지 않고 대기 목록에 돌려놓음
				ChainPendingNodeList(node);
			}

			// 3) 내가 꺼낸 노드는 삭제
			//    _popCount가 1이었던 시점에 이 노드는 이미 _head에서 분리된 상태였으므로,
			//    그 이후에 들어온 스레드는 이 노드를 볼 수 없음
			delete oldHead;
		}
		else
		{
			// 다른 스레드도 TryPop 안에 있음
			// 그 스레드가 oldHead를 보고 있을 수 있으므로 지금 삭제하지 않고 대기 목록에 추가
			ChainPendingNode(oldHead);
			--_popCount;
		}
	}

	// first ~ last로 이어진 노드 묶음을 _pendingList 앞쪽에 붙임 (Push와 같은 방식)
	void ChainPendingNodeList(Node* first, Node* last)
	{
		// 묶음의 마지막 노드가 현재 대기 목록의 첫 번째 노드를 가리키도록 설정
		last->next = _pendingList;

		// _pendingList가 아직 last->next와 같다면 _pendingList를 first로 교체
		while (_pendingList.compare_exchange_weak(last->next, first) == false)
		{
			// 실패하면 last->next에 최신 _pendingList 값이 자동으로 들어감 -> 다시 시도
		}
	}

	// 노드 목록의 마지막 노드를 찾은 뒤 _pendingList에 붙임
	// node는 nullptr이면 안 됨 (호출하는 쪽에서 확인)
	void ChainPendingNodeList(Node* node)
	{
		Node* last = node;
		while (last->next)
			last = last->next;

		ChainPendingNodeList(node, last);
	}

	// 노드 하나를 _pendingList에 붙임 (처음과 끝이 같은 묶음으로 처리)
	void ChainPendingNode(Node* node)
	{
		ChainPendingNodeList(node, node);
	}

	// 연결된 노드들을 끝까지 따라가며 모두 삭제
	static void DeleteNodes(Node* node)
	{
		while (node)
		{
			Node* next = node->next; // 삭제 전에 다음 노드 주소를 먼저 저장
			delete node;
			node = next;
		}
	}

private:
	atomic<Node*> _head{ nullptr };

	atomic<uint32_t> _popCount = 0;            // TryPop을 실행 중인 스레드 수
	atomic<Node*> _pendingList{ nullptr };     // [수정 3] 삭제를 미뤄둔 노드 목록의 첫 번째 노드
};