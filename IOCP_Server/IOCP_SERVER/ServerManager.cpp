#include "ServerManager.h"

ServerManager::ServerManager()
	:_listenSocket(INVALID_SOCKET)
	, _completionPortHandle(INVALID_HANDLE_VALUE)
	, _isThreadStart(false)
{
	_dataInfoList.clear();
	_workerThreadList.clear();
}

ServerManager:: ~ServerManager()
{
	WSACleanup();
}

const bool ServerManager::init()
{
	WSADATA wsaData;

	int rv = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (rv != 0)
		return false;

	_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (_listenSocket == INVALID_SOCKET)
		return false;

	SOCKADDR_IN	serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	rv = bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN));
	if (rv != 0)
		return false;

	rv = listen(_listenSocket, IO_QUEUE_MAXCOUNT);
	if (rv != 0)
		return false;

	_completionPortHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, IO_THREAD_MAXCOUNT);
	if (_completionPortHandle == nullptr)
		return false;

	auto handle = CreateIoCompletionPort((HANDLE)_listenSocket, _completionPortHandle, (UINT32)0, 0);
	if (handle == nullptr)
		return false;

	_packetManager = std::make_unique<PacketManagerInServer>();
	auto sendPacketFunc = [&](const unsigned int index, const unsigned int size, char* data)
	{
		sendMessage(index, size, data);
	};
	_packetManager->init(CLIENT_MAXCOUNT);
	_packetManager->sendPacketFunc = sendPacketFunc;

	for (unsigned int i = 0; i < CLIENT_MAXCOUNT; ++i)
	{
		auto dataInfo = new DataInfo(i, _completionPortHandle);
		_dataInfoList.push_back(dataInfo);
	}

	return true;
}

void ServerManager::start()
{
	_packetManager->start();

	for (UINT32 i = 0; i < IO_THREAD_MAXCOUNT; i++)
	{
		_workerThreadList.emplace_back([this]() { wokerThread(); });
	}

	_accepterThread = std::thread([this]() { accepterThread(); });
}

void ServerManager::end()
{
	_packetManager->end();

	CloseHandle(_completionPortHandle);

	_isThreadStart = false;
	for (auto& th : _workerThreadList)
	{
		if (th.joinable() == true)
			th.join();
	}

	closesocket(_listenSocket);

	if (_accepterThread.joinable() == true)
		_accepterThread.join();
}

void ServerManager::wokerThread()
{
	bool rv = true;
	DataInfo* dataInfo = nullptr;
	DWORD dataSize = 0;
	LPOVERLAPPED lpOverlapped = nullptr;

	while (_isThreadStart == true)
	{
		rv = GetQueuedCompletionStatus(_completionPortHandle, &dataSize, (PULONG_PTR)&dataInfo, &lpOverlapped, INFINITE);

		if (rv == true && dataSize == 0 && lpOverlapped == nullptr)
			continue;

		if (lpOverlapped == nullptr)
			continue;

		auto pOverlappedInfo = (OverlappedInfo*)lpOverlapped;

		if (rv == false || (dataSize == 0 && pOverlappedInfo->_state != eIOState::ACCEPT))
		{
			closeSocket(dataInfo, true);
			continue;
		}

		switch (pOverlappedInfo->_state)
		{
		case eIOState::ACCEPT:
		{
			dataInfo = getClientInfo(pOverlappedInfo->_sessionIndex);
			if (dataInfo->completeAccept() == true)
			{
				completeConnect(dataInfo->getIndex());
			}
			else
			{
				closeSocket(dataInfo, true);
			}
		}
		break;
		case eIOState::RECV:
		{
			completeReceive(dataInfo->getIndex(), dataSize, dataInfo->getRecvBuffer());
			dataInfo->bindRecieve();
		}
		break;
		case eIOState::SEND:
		{
			dataInfo->completeSend(dataSize);
		}
		break;
		default:
			break;
		}
	}
}

void ServerManager::accepterThread()
{
	while (_isThreadStart == true)
	{
		time_t curTime;
		time(&curTime);

		for (auto data : _dataInfoList)
		{
			if (data->isAccept() == true)
				continue;

			if (curTime < data->getLastAcceptTime())
				continue;

			data->readyAccept(_listenSocket, curTime);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}
}

void ServerManager::closeSocket(DataInfo* dataInfo, const bool isForce)
{
	if (dataInfo->isAccept() == false)
		return;

	auto clientIndex = dataInfo->getIndex();
	dataInfo->close(isForce);
	completeClose(clientIndex);
}

void ServerManager::completeConnect(const unsigned int index)
{
	PacketInfo packet{ index, (UINT16)PACKET_ID::REQ_USER_CONNECT, 0 };
	_packetManager->pushSystemPacket(packet);
}

void ServerManager::completeClose(const unsigned int index)
{
	PacketInfo packet{ index, (UINT16)PACKET_ID::REQ_USER_DISCONNECT, 0 };
	_packetManager->pushSystemPacket(packet);
}

void ServerManager::completeReceive(const unsigned int index, const unsigned int size, char* data)
{
	_packetManager->receivePacketData(index, size, data);
}

const bool ServerManager::sendMessage(const unsigned int index, const unsigned int size, char* data)
{
	DataInfo* dataInfo = getClientInfo(index);
	return dataInfo->sendMessage(size, data);
}

DataInfo* ServerManager::getClientInfo(const unsigned int index)
{
	if (index < 0 || index >= _dataInfoList.size())
		return nullptr;
	return _dataInfoList[index];
}