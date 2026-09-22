#pragma once
#include <mutex>
#include "AccountManager.h"

class User {

};

class UserManager
{
public:
	static UserManager* instance()
	{
		static UserManager instance;
		return &instance;
	}

	User* GetUser(int id)
	{
		std::lock_guard<std::mutex> guard(_mutex);
		return nullptr;
	}

	void ProcessSave();

private:
	std::mutex _mutex;
};
