#include "PacketManagerInServer.h"

PacketManagerInServer::PacketManagerInServer()
	:_isThreadStart(false)
	, _userManager(nullptr)
{
	_packetFuntionList.clear();
	_packetUserIndexList.clear();
}

PacketManagerInServer:: ~PacketManagerInServer()
{
}

const bool PacketManagerInServer::init(const unsigned short clientMaxCount)
{
	settingPacketFunc();
	_userManager = new UserManagerInServer;
	_userManager->init(clientMaxCount);

	return true;
}

const bool PacketManagerInServer::start()
{
	_isThreadStart = true;
	for (UINT32 i = 0; i < 5; i++)
	{
		_processThreadList.emplace_back([this]() { processPacket(); });
	}
	return true;
}

void PacketManagerInServer::end()
{
	_isThreadStart = false;

	for (auto& th : _processThreadList)
	{
		if (th.joinable() == true)
			th.join();
	}
}

void PacketManagerInServer::processPacket()
{
	while (_isThreadStart == true)
	{
		const unsigned int index = dequePacketData();
		if (index == 0)
			continue;

		// 직렬화 처리
		{
			std::lock_guard<std::mutex> guard(_lock);
			std::thread::id threadId = std::this_thread::get_id();

			auto findItor = _serialLizePacket.find(index);
			if (findItor == _serialLizePacket.end())
				continue;

			if (findItor->second._threadId != threadId)
				continue;

			PacketInfo packetData = findItor->second._packetList.front();
			processRecvPacket(packetData._index, packetData._packetId, packetData._dataSize, packetData._data);
		}
	}
}

void PacketManagerInServer::processRecvPacket(const unsigned int index, static const unsigned short packetId, const unsigned int size, char* data)
{
	auto iter = _packetFuntionList.find(packetId);
	if (iter != _packetFuntionList.end())
	{
		(this->*(iter->second))(index, size, data);
	}
}

void PacketManagerInServer::receivePacketData(const unsigned int index, const unsigned int size, char* data)
{
	auto pHeader = (Packet_Header*)&data;

	PacketInfo packetInfo;
	packetInfo._packetId = pHeader->_packetId;
	packetInfo._dataSize = pHeader->_packetLength;
	packetInfo._data = data;

	enqueuePacketData(index, packetInfo);
}

void PacketManagerInServer::enqueuePacketData(const unsigned int index, const PacketInfo packInfo)
{
	std::lock_guard<std::mutex> guard(_lock);
	_packetUserIndexList.push_back(index);
	auto findItor = _serialLizePacket.find(index);
	if (findItor == _serialLizePacket.end())
	{
		findItor->second._packetList.push(packInfo);
	}
	else
	{
		SerializePacketInfo serializeinfo;
		serializeinfo._threadId = std::thread::id();
		serializeinfo._packetList.push(packInfo);
		_serialLizePacket.insert(std::make_pair(index, serializeinfo));
	}
}

const unsigned int PacketManagerInServer::dequePacketData()
{
	unsigned int userIndex = 0;

	{
		std::lock_guard<std::mutex> guard(_lock);
		if (_packetUserIndexList.empty())
			return 0;

		std::thread::id curThreadId = std::this_thread::get_id();
		for (unsigned int ii = 0; ii < _packetUserIndexList.size(); ++ii)
		{
			userIndex = _packetUserIndexList[ii];
			auto findItor = _serialLizePacket.find(userIndex);
			if (findItor == _serialLizePacket.end())
			{
				// TODO 직렬화가 필요없는패킷들처리. 
				_packetUserIndexList.erase(_packetUserIndexList.begin() + ii);
				return 0;
			}

			// 어느 스레드에서 처리할지 세팅 
			if (findItor->second._threadId == std::thread::id())
			{
				findItor->second._threadId = std::this_thread::get_id();
				_packetUserIndexList.erase(_packetUserIndexList.begin() + ii);
				return userIndex;
			}

			if (findItor->second._threadId == curThreadId)
			{
				_packetUserIndexList.erase(_packetUserIndexList.begin() + ii);
				return userIndex;
			}
		}
	}

	return 0;
}

void PacketManagerInServer::pushSystemPacket(PacketInfo packet)
{
	std::lock_guard<std::mutex> guard(_lock);
	enqueuePacketData(packet._index, packet);
}

void PacketManagerInServer::settingPacketFunc()
{
	_packetFuntionList = std::unordered_map<int, PACKET_FUNCTION>();
	_packetFuntionList[(int)PACKET_ID::REQ_USER_CONNECT] = &PacketManagerInServer::reqConnectProccess;
	_packetFuntionList[(int)PACKET_ID::REQ_USER_DISCONNECT] = &PacketManagerInServer::reqDisConnectProccess;
	_packetFuntionList[(int)PACKET_ID::REQ_END] = &PacketManagerInServer::reqEndProccess;

	_packetFuntionList[(int)PACKET_ID::REQ_LOGIN] = &PacketManagerInServer::reqLoginProccess;
}

void PacketManagerInServer::reqConnectProccess(const unsigned int index, const unsigned int size, char* data)
{}
void PacketManagerInServer::reqDisConnectProccess(const unsigned int index, const unsigned int size, char* data)
{}
void PacketManagerInServer::reqEndProccess(const unsigned int index, const unsigned int size, char* data)
{}
void PacketManagerInServer::reqLoginProccess(const unsigned int index, const unsigned int size, char* data)
{
	auto packet = reinterpret_cast<ReqLogin*>(data);
	auto userInfo = _userManager->getUserByIndex(index);
	// TODO
}
