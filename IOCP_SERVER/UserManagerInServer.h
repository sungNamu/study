#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "Packet.h"

class User
{
public:
	User(void);
	~User(void);

	void									init(const unsigned int index);

private:
	//<----------------- OVER SIZE ----------------->
	//<----------------- 8byte SIZE ----------------->
	//<----------------- 4byte SIZE ----------------->
	unsigned int							_index;
	//<----------------- 2byte SIZE ----------------->
	//<----------------- 1byte SIZE ----------------->
};

class UserManagerInServer
{
public:
	UserManagerInServer(void);
	~UserManagerInServer(void);

	void									init(const unsigned int maxUserCount);
	User* getUserByIndex(const unsigned int index);
private:

	//<----------------- OVER SIZE ----------------->
	std::vector<User*>						_userPool;
	//<----------------- 8byte SIZE ----------------->
	//<----------------- 4byte SIZE ----------------->
	//<----------------- 2byte SIZE ----------------->
	//<----------------- 1byte SIZE ----------------->
};

