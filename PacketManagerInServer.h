#pragma once

#include <unordered_map>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include "Packet.h"
#include "UserManagerInServer.h"

class PacketManagerInServer
{
public:
	PacketManagerInServer(void);
	~PacketManagerInServer(void);

	const bool												init(const unsigned short clientMaxCount);
	const bool												start();
	void													end();
	void													processPacket();
	void													processRecvPacket(const unsigned int index, static const unsigned short packetId, const unsigned int size, char* data);
	void													receivePacketData(const unsigned int index, const unsigned int size, char* data);
	void													enqueuePacketData(const unsigned int index, const PacketInfo packInfo);
	const unsigned int										dequePacketData();
	void													pushSystemPacket(PacketInfo packet);

	void													settingPacketFunc();
	void													reqConnectProccess(const unsigned int index, const unsigned int size, char* data);
	void													reqDisConnectProccess(const unsigned int index, const unsigned int size, char* data);
	void													reqEndProccess(const unsigned int index, const unsigned int size, char* data);
	void													reqLoginProccess(const unsigned int index, const unsigned int size, char* data);


	std::function<void(const unsigned int index, const unsigned int size, char* data)> sendPacketFunc;
private:
	typedef void(PacketManagerInServer::* PACKET_FUNCTION)(const unsigned int, const unsigned int, char*);
	//<----------------- OVER SIZE ----------------->
	std::mutex												_lock;
	std::vector<std::thread>								_processThreadList;
	std::unordered_map<int, PACKET_FUNCTION>				_packetFuntionList;
	std::vector<unsigned int>								_packetUserIndexList;
	std::unordered_map<unsigned int, SerializePacketInfo>	_serialLizePacket;

	//<----------------- 8byte SIZE ----------------->
	UserManagerInServer* _userManager;
	//<----------------- 4byte SIZE ----------------->
	//<----------------- 2byte SIZE ----------------->
	//<----------------- 1byte SIZE ----------------->
	bool													_isThreadStart;

};
