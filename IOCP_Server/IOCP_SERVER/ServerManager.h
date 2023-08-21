#pragma once
#include <WinSock2.h>
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock.lib")

#include <thread>
#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#include "PacketManagerInServer.h"
#include "DataInfo.h"
#include "Packet.h"

static const unsigned short	SERVER_PORT			= 8888;
static const unsigned short	CLIENT_MAXCOUNT		= 2;
static const unsigned int	IO_THREAD_MAXCOUNT	= 4;
static const unsigned int	IO_QUEUE_MAXCOUNT	= 5;

class ServerManager
{
public:
	ServerManager(void);
	~ServerManager(void);

	const bool								init();
	void 									start();
	void									end();
	void									wokerThread();
	void									accepterThread();
	void									closeSocket(DataInfo* dataInfo, const bool isForce);
	void									completeConnect(const unsigned int index);
	void									completeClose(const unsigned int index);
	void									completeReceive(const unsigned int index, const unsigned int size, char* data);
	const bool								sendMessage(const unsigned int index, const unsigned int size, char* data);
	DataInfo* getClientInfo(const unsigned int index);
private:
	//<----------------- OVER SIZE ----------------->
	std::vector<DataInfo*>					_dataInfoList;
	std::vector<std::thread>				_workerThreadList;
	std::thread								_accepterThread;
	//<----------------- 8byte SIZE ----------------->
	std::unique_ptr<PacketManagerInServer>	_packetManager;
	SOCKET									_listenSocket;
	HANDLE									_completionPortHandle;
	//<----------------- 4byte SIZE ----------------->
	//<----------------- 2byte SIZE ----------------->
	//<----------------- 1byte SIZE ----------------->
	bool									_isThreadStart;
};

