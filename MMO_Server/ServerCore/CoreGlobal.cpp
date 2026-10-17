#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"


ThreadManager* GThreadManager = nullptr;

class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
	}

private:

};

CoreGlobal CoreGlobal;

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
