#include "UserManagerInServer.h"

User::User()
	:_index(0)
{

}

User:: ~User()
{
}

void User::init(const unsigned int index)
{
	_index = index;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

UserManagerInServer::UserManagerInServer()
{
	_userPool.clear();
}

UserManagerInServer:: ~UserManagerInServer()
{
}

void UserManagerInServer::init(const unsigned int maxUserCount)
{
	_userPool.reserve(maxUserCount);

	for (auto i = 0; i < maxUserCount; i++)
	{
		_userPool.push_back(new User());
		_userPool[i]->init(i);
	}
}

User* UserManagerInServer::getUserByIndex(const unsigned int index)
{
	return _userPool[index];
}