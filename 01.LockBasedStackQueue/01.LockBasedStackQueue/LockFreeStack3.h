#pragma once
#include <stack>
#include <atomic>
#include <mutex>
#include <condition_variable>
using namespace std;
//using int32 = int32_t;

/*
 =====================================================================================
 [Lock-Free Stack3 - 분할 참조 카운트(Split Reference Count) 방식]

 ■ LockFreeStack2의 한계
	- TryPop 안에 "혼자인 순간"이 와야만 노드를 삭제할 수 있어서,
	  TryPop이 계속 겹쳐서 호출되면 삭제 대기 목록이 계속 길어졌습니다.
	- 노드의 next를 스택 연결과 삭제 대기 목록 연결에 함께 써서 데이터 레이스가 있었습니다.

 ■ 해결 아이디어
	- 스택 전체가 아니라 "노드 하나하나"에 대해, 그 노드를 보고 있는 스레드 수를 셉니다.
	- 마지막으로 그 노드에서 손을 떼는 스레드가 노드를 삭제합니다.
	- 카운트를 두 개로 나눠서 관리합니다.

 ■ 두 개의 카운트
	- externalCount (외부 카운트, CountedNodePtr 안에 있음)
		- head 포인터와 한 묶음으로 저장되어, 포인터와 카운트가 한 번의 CAS로 함께 바뀝니다.
		- "이 노드를 보러 들어온 스레드 수 + head의 참조 1"을 뜻합니다.
		- Push 시 1 (head가 이 노드를 가리키는 참조)
		- TryPop에 들어온 스레드가 노드를 읽기 전에 +1
		- 값이 늘어나기만 하고 줄어들지 않습니다.
	- internalCount (내부 카운트, Node 안에 있음)
		- "이 노드에서 손을 뗀 스레드 수"를 기록합니다.
		- CAS에 실패한 스레드는 -1
		- CAS에 성공한 스레드는 (externalCount - 2)를 더함
		- 이 값이 0이 되는 순간 = 들어온 스레드가 모두 나간 순간 -> 그때 삭제

 ■ 동작 예시 (스레드 A, B, C가 같은 노드를 보고 있음)
	- Push 직후 externalCount = 1
	- A, B, C가 차례로 IncreaseHeadCount -> externalCount = 4
	- A가 CAS에 성공 (B, C는 실패)
		- A: countIncrease = 4 - 2 = 2 (head의 참조 1과 A 자신 1을 뺀 나머지 = B, C)
	- 경우 1) A가 먼저 정리: internal 0 -> 2, B: 2 -> 1, C: 1 -> 0 -> C가 삭제
	- 경우 2) B, C가 먼저 정리: internal 0 -> -1 -> -2, A: -2 -> 0 -> A가 삭제
	- 어느 순서로 끝나도 마지막 스레드 한 명만 삭제합니다.

 ■ 이전 버전 대비 나아진 점
	- 다른 스레드가 계속 TryPop을 호출해도, 각 노드는 그 노드를 본 스레드가 모두 나가면 삭제됩니다.
	- next는 Push에서 노드를 head에 연결하기 전에만 쓰이고 이후에는 읽기만 하므로 데이터 레이스가 없습니다.
	- 노드가 삭제되지 않는 동안에는 같은 주소가 재사용되지 않으므로 ABA도 생기지 않습니다.
	- 값을 shared_ptr<T>로 돌려주므로, 값 복사 중 예외가 나서 데이터가 사라지는 문제가 없습니다.

 ■ 존재할 수 있는 문제점
	1. 실제로는 락프리가 아닐 수 있음
		- CountedNodePtr는 x64에서 16바이트입니다 (int32 4바이트 + 패딩 4바이트 + 포인터 8바이트).
		- 16바이트 atomic은 구현에 따라 내부에서 락을 사용합니다.
		  (GCC x64에서 _head.is_lock_free()가 false로 확인됨)
		- 사용하는 컴파일러에서 is_lock_free()로 확인이 필요합니다.
		- 해결 방법 예: x64 포인터는 하위 48비트만 쓰므로, 카운트를 상위 16비트에 넣어
		  8바이트(atomic<uint64_t>)로 압축합니다.
	2. 구조체 패딩
		- externalCount와 ptr 사이에 4바이트 패딩이 있습니다.
		- C++17까지는 compare_exchange가 패딩 바이트까지 비교할 수 있어서,
		  값이 같아도 CAS가 실패할 수 있습니다.
		- 실제 확인 결과 (GCC 13.3, x64, C++17):
		  단일 스레드에서 Push를 한 번 호출했는데 CAS가 계속 실패해 무한 루프에 빠졌습니다.
		  같은 코드를 C++20으로 빌드하거나, externalCount를 int64_t로 바꿔 패딩을 없애면 정상 동작했습니다.
		- C++20부터는 패딩을 비교하지 않도록 표준이 바뀌었습니다.
		- 해결 방법 예: externalCount를 8바이트 타입으로 바꿔 패딩을 없애거나, C++20 이상으로 빌드
	3. 빈 스택에서 TryPop을 반복하면 카운트가 계속 증가함
		- 스택이 비어 있어도 IncreaseHeadCount가 먼저 실행되어 {count, nullptr}의 count가 늘어납니다.
		- 이 값은 다음 Push된 노드의 next에 들어갔다가 다시 head로 돌아오므로 초기화되지 않습니다.
		- 약 21억 번 뒤 int32 오버플로가 나며, 부호 있는 정수 오버플로는 정의되지 않은 동작입니다.
		- 빈 스택 확인만 해도 매번 CAS 쓰기가 일어나 경합이 생깁니다.
		- 해결 방법 예: 루프 시작 시 oldHead.ptr == nullptr이면 카운트를 올리기 전에 반환
		  (역참조 없이 nullptr 여부만 확인하므로 안전)
	4. head 경합
		- 모든 TryPop이 노드를 읽기 전에 head에 CAS 쓰기를 하므로,
		  스레드가 많으면 head가 있는 캐시 라인에 경합이 몰립니다.
	5. 소멸자 없음
		- 스택에 남은 노드가 해제되지 않습니다.
	6. 헤더 관련
		- shared_ptr, make_shared에 필요한 <memory>와 int32_t에 필요한 <cstdint>가 직접 포함되어 있지 않습니다.
		  (지금은 다른 헤더를 통해 간접 포함되어 컴파일되지만, 환경에 따라 오류가 날 수 있음)
		- <stack>, <mutex>, <condition_variable>은 사용하지 않습니다.
		- 헤더 파일의 using namespace std;는 이 헤더를 포함하는 모든 파일에 영향을 줍니다.
 =====================================================================================
*/

template<typename T>
class LockFreeStack3
{
	struct Node;

	// head(또는 next)와 외부 카운트를 한 묶음으로 저장
	// 묶음 전체를 한 번에 CAS 하므로, 포인터와 카운트가 항상 함께 바뀜
	struct CountedNodePtr
	{
		int32_t externalCount = 0; // 이 노드를 보러 들어온 스레드 수 + head의 참조 1
		Node* ptr = nullptr;
	};

	struct Node
	{
		// 데이터를 미리 shared_ptr로 만들어 둠
		// make_shared에서 예외가 나도 아직 스택에 연결되기 전이므로 스택 상태는 그대로임
		Node(const T& value) : data(make_shared<T>(value))
		{

		}

		shared_ptr<T> data;
		atomic<int32_t> internalCount = 0; // 이 노드에서 손을 뗀 스레드 기록 (0이 되면 삭제)
		CountedNodePtr next;             // Push에서 연결 전에만 쓰고, 이후에는 읽기만 함
	};

public:
	// [][][][][][][]
	// [head]
	void Push(const T& value)
	{
		// 1) 새 노드 생성. externalCount = 1은 "head가 이 노드를 가리키는 참조"
		CountedNodePtr node;
		node.ptr = new Node(value);
		node.externalCount = 1;
		// [!]
		// 2) 새 노드의 next가 현재 head(포인터 + 카운트)를 가리키도록 설정
		node.ptr->next = _head;

		// 3) _head가 아직 node.ptr->next와 같다면 _head를 새 노드로 교체
		//    실패하면 node.ptr->next에 최신 _head 값이 자동으로 들어감 -> 다시 시도
		while (_head.compare_exchange_weak(node.ptr->next, node) == false)
		{
		}
	}

	// [][][][][][][]
	// [head]
	shared_ptr<T> TryPop()
	{
		CountedNodePtr oldHead = _head;
		while (true)
		{
			// 1) 노드를 읽기 전에 head의 외부 카운트를 +1 해서 "내가 이 노드를 보고 있다"고 알림
			//    성공하면 oldHead에는 내가 올린 카운트까지 반영된 값이 들어 있음
			IncreaseHeadCount(oldHead);

			// 2) 이제 externalCount에 내가 포함되어 있으므로,
			//    내가 손을 떼기 전까지 이 노드는 삭제되지 않음 -> ptr 역참조가 안전함
			Node* ptr = oldHead.ptr;

			// 3) 스택이 비어 있음
			//    [문제점 3] 이 경우에도 1)에서 nullptr head의 카운트를 이미 올린 상태임
			if (ptr == nullptr)
				return shared_ptr<T>();

			// 4) head를 다음 노드로 교체 시도
			//    포인터뿐 아니라 externalCount까지 함께 비교하므로,
			//    그 사이에 다른 스레드가 카운트를 올렸어도 실패함
			//    -> 성공한 스레드의 oldHead.externalCount는 이 노드의 최종 외부 카운트임
			if (_head.compare_exchange_strong(oldHead, ptr->next))
			{
				// 5-1) 노드를 꺼내는 데 성공함
				//      데이터는 이 스레드만 가져가므로 swap으로 소유권을 넘겨받음
				shared_ptr<T> res;
				res.swap(ptr->data);

				// head의 참조 1과 나 자신 1을 뺀 나머지 = 아직 이 노드를 보고 있는 다른 스레드 수
				// 예) Push로 1, 스레드 3개가 +1씩 -> externalCount = 4 -> countIncrease = 2
				const int32_t countIncrease = oldHead.externalCount - 2;

				// 내부 카운트에 더한 결과가 0이면(이전 값이 -countIncrease)
				// 다른 스레드가 이미 모두 손을 뗀 상태 -> 내가 마지막이므로 삭제
				if (ptr->internalCount.fetch_add(countIncrease) == -countIncrease)
					delete ptr;

				return res;
			}
			else if (ptr->internalCount.fetch_sub(1) == 1)
			{
				// 5-2) 다른 스레드가 먼저 head를 바꿔서 실패함 -> 이 노드에서 손을 뗌 (-1)
				//      뺀 결과가 0이면(이전 값이 1) 꺼낸 스레드는 이미 정리를 마쳤고
				//      내가 마지막으로 손을 뗀 스레드이므로 삭제
				delete ptr;
			}

			// 6) 실패한 경우 oldHead에는 CAS가 넣어준 최신 head 값이 들어 있으므로 1)부터 다시 시도
		}
	}

private:
	// head의 외부 카운트를 +1 하는 함수
	// 포인터는 그대로 두고 카운트만 올린 값으로 CAS
	void IncreaseHeadCount(CountedNodePtr& oldCounter)
	{
		while (true)
		{
			CountedNodePtr newCounter = oldCounter;
			newCounter.externalCount++;

			// _head가 아직 oldCounter와 같다면 카운트를 올린 newCounter로 교체
			// 실패하면 oldCounter에 최신 _head 값이 자동으로 들어감 -> 다시 시도
			// (그 사이 head가 다른 노드로 바뀌었다면, 바뀐 노드의 카운트를 올리게 됨)
			if (_head.compare_exchange_strong(oldCounter, newCounter))
			{
				// 호출한 쪽이 올라간 카운트 값을 알 수 있도록 반영
				oldCounter.externalCount = newCounter.externalCount;
				break;
			}
		}
	}

private:
	// [문제점 1] CountedNodePtr는 16바이트 -> 구현에 따라 내부에서 락을 사용할 수 있음
	// [문제점 2] externalCount(4바이트)와 ptr(8바이트) 사이에 4바이트 패딩이 있음
	atomic<CountedNodePtr> _head;
};