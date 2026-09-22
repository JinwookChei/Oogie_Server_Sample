#include "UserManager.h"

void UserManager::ProcessSave()
{
	std::lock_guard<std::mutex> guard(_mutex);

	Account* account = AccountManager::instance()->GetAccount(100);
}
