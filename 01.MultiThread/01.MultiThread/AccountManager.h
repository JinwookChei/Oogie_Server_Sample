#pragma once
#include <mutex>
#include "UserManager.h"

class User;
class UserManager;

class Account {

};

class AccountManager 
{
public:
	static AccountManager* instance() 
	{
		static AccountManager instance;
		return &instance;
	}

	Account* GetAccount(int id) 
	{
		std::lock_guard<std::mutex> guard(_mutex);
		return nullptr;
	}

	void ProcessLogin();

private:
	std::mutex _mutex;
	//std::map<int, Account>

};
