#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "DeadLockProfiler.h"


ThreadManager* GThreadManager = nullptr;
DeadLockProfiler* GDeadLockProfiler = nullptr;	// Lock.cpp에서 _DEBUG일 때 사용하는 데드락 탐지기

/*
	[CoreGlobal]
	- 전역 매니저들의 생성/소멸을 한 곳에서 관리하는 클래스.
	- 전역 객체(GCoreGlobal)로 하나 만들어 두면 main() 진입 전에 생성자가,
	  프로그램 종료 시 소멸자가 자동으로 호출된다.
*/
class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
		GDeadLockProfiler = new DeadLockProfiler();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GDeadLockProfiler;
	}

private:

};

CoreGlobal GCoreGlobal;

//
//CoreGlobal::CoreGlobal()
//{
//	GThreadManager = new ThreadManager();
//}
//
//CoreGlobal::~CoreGlobal()
//{
//	delete GThreadManager;
//}
