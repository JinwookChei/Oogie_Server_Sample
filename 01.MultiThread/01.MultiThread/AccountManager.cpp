#include "AccountManager.h"
#include "UserManager.h"

void AccountManager::ProcessLogin()
{
	std::lock_guard<std::mutex> guard(_mutex);
	User* user = UserManager::instance()->GetUser(100);
}
