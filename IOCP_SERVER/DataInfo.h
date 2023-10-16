#pragma once

#include <Ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <mutex>
#include <queue>
#include "Packet.h"

class DataInfo
{
public:
	DataInfo(const unsigned int index, HANDLE iocpHandle);
	~DataInfo(void);

	const bool							isAccept();
	const bool							readyAccept(SOCKET listenSocket, const unsigned __int64 curTime);
	const bool							completeAccept();
	const bool							completeConnect(HANDLE iocpHandle, SOCKET socket);
	void								completeSend(const unsigned int size);
	const bool							bindRecieve();
	const bool							sendMessage(const unsigned int size, char* data);
	const bool							sendIO();
	void								close(const bool isForce);
	const const unsigned __int64		getLastAcceptTime();
	const unsigned int					getIndex();
	char*								getRecvBuffer();

private:

private:
	//<----------------- OVER SIZE ----------------->
	std::mutex							_lock;
	OverlappedInfo						_accepOverlappedEx;
	OverlappedInfo						_recvOverlappedEx;
	char								_acceptBuf[SOCK_BUF];
	char								_recvBuf[SOCK_BUF];
	std::queue<OverlappedInfo*>			_sendDataList;
	//<----------------- 8byte SIZE ----------------->
	time_t								_lastAcceptTime;
	HANDLE								_iOCPHandle;
	SOCKET								_socket;
	//<----------------- 4byte SIZE ----------------->
	unsigned int						_Index;
	//<----------------- 2byte SIZE ----------------->
	//<----------------- 1byte SIZE ----------------->
	bool								_isAccept;
};

