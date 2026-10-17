#pragma once
#include <stack>
#include <atomic>
#include <mutex>
#include <condition_variable>
using namespace std;

/*
 =====================================================================================
 [Lock-Free Stack1 구현체 검토 및 주요 문제점 정리]
 * 학습 목적으로 문제를 일부러 남겨둔 구현입니다.

 1. Use-After-Free (댕글링 포인터)
	- TryPop에서 스레드 A가 `_head`를 로드해 `oldHead`를 얻은 직후,
	  스레드 B가 같은 노드를 pop하고 `delete oldHead;`로 메모리를 해제할 수 있습니다.
	- 이후 스레드 A가 `compare_exchange_weak(oldHead, oldHead->_next)`를 실행하면
	  해제된 메모리인 `oldHead->_next`를 읽게 됩니다.
	- 문제가 되는 시점은 CAS 자체가 아니라, CAS의 인자인 `oldHead->_next`를 읽는 순간입니다.
	  이 값은 CAS 호출 전에 평가되므로, CAS가 실패해서 결과를 쓰지 않더라도
	  해제된 메모리를 읽은 시점에 이미 정의되지 않은 동작(Undefined Behavior)입니다.
	- 해제된 페이지가 OS에 반환된 상태라면 Access Violation이 발생하고,
	  그렇지 않으면 쓰레기값을 읽고 그대로 진행될 수 있어 재현이 일정하지 않습니다.

 2. ABA 문제
	- CAS가 포인터 주소값만 비교하기 때문에 발생합니다.
	- 스레드 A가 `oldHead`를 읽은 뒤 다른 스레드가 노드를 빼내고,
	  해제된 주소가 새 노드에 재할당되어 다시 `_head`에 들어오면,
	  CAS는 값이 바뀌지 않았다고 판단해 성공합니다.
	- 발생 순서 예시
		(1) 스택 상태: A -> B -> C
		(2) 스레드 1: oldHead = A, oldHead->_next = B 까지 읽고 멈춤
		(3) 스레드 2: A pop(delete), B pop(delete), 새 노드 push.
			이때 allocator가 A의 주소를 재사용함.
			스택 상태: A' -> C (A'은 A와 주소가 같은 새 노드)
		(4) 스레드 1: CAS에서 `_head == A`이므로 성공하고 `_head = B`로 설정
		(5) 결과: `_head`가 이미 해제된 B를 가리키고, A'과 C는 스택에서 사라짐
 =====================================================================================
*/
template<typename T>
class LockFreeStack
{
	struct Node
	{
		Node(const T& value) : data(value), _next(nullptr) {}

		T data;
		Node* _next;
	};

public:
	void Push(const T& value)
	{
		Node* node = new Node(value);
		node->_next = _head.load();

		// _head가 node->_next와 같다면 _head를 node로 교체
		while (!_head.compare_exchange_weak(node->_next, node))
		{
			// 실패 시 node->_next가 최신 _head로 자동 갱신됨
		}
	}

	bool TryPop(T& value)
	{
		Node* oldHead = _head.load();

		// oldHead가 유효하고, _head가 여전히 oldHead라면 oldHead->_next로 교체
		while (oldHead && !_head.compare_exchange_weak(oldHead, oldHead->_next))
		{
			// 실패 시 oldHead가 최신 _head로 자동 갱신됨
		}
			 
		if (oldHead == nullptr)
			return false;

		value = oldHead->data;
		delete oldHead;

		return true;
	}

private:
	// 헤드는 노드가 아니라 '노드의 포인터'여야 합니다.
	atomic<Node*> _head{ nullptr };
};