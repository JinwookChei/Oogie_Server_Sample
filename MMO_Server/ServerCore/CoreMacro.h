#pragma once
#define OUT


/*
		   [ Lock ]
	- READ_LOCK / WRITE_LOCK은 락 이름으로 typeid(this).name()을 넘긴다.
	  · this의 타입 이름(예: "class PlayerManager *")이므로 클래스마다 고유한 이름이 된다.
	  · 컴파일러가 만들어 둔 문자열이라 같은 클래스면 항상 같은 주소 → DeadLockProfiler가 포인터로 비교 가능.
	  · 단, 같은 클래스의 인스턴스 여러 개, 혹은 _locks[0], _locks[1]은 모두 같은 이름으로 취급된다.
*/
#define USE_MANY_LOCKS(count)	Lock _locks[count];
#define USE_LOCK				USE_MANY_LOCKS(1)
#define READ_LOCK_IDX(idx)		ReadLockGuard readLockGuard_##idx(_locks[idx], typeid(this).name());
#define READ_LOCK				READ_LOCK_IDX(0)
#define WRITE_LOCK_IDX(idx)		WriteLockGuard writeLockGuard_##idx(_locks[idx], typeid(this).name());
#define WRITE_LOCK				WRITE_LOCK_IDX(0)


/*
		   [ Crash ]
*/

#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(crash != nullptr);	\
	*crash = 0xDEADBEEF;					\
}


#define ASSERT_CRASH(expr)			\
{									\
	if(!(expr))						\
	{								\
		CRASH("ASSERT_CRASH")		\
		__analysis_assume(expr);	\
	}								\
}									\