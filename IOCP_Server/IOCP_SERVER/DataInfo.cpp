#include "DataInfo.h"

DataInfo::DataInfo(const unsigned int index, HANDLE iocpHandle)
	:_Index(index)
	, _socket(INVALID_SOCKET)
	, _iOCPHandle(iocpHandle)
	, _isAccept(false)
	, _lastAcceptTime(0)
{
	ZeroMemory(&_accepOverlappedEx, sizeof(OverlappedInfo));
	ZeroMemory(&_recvOverlappedEx, sizeof(OverlappedInfo));

	ZeroMemory(&_acceptBuf, sizeof(SOCK_BUF));
	ZeroMemory(&_recvBuf, sizeof(SOCK_BUF));
}

DataInfo:: ~DataInfo()
{
}

const bool DataInfo::isAccept()
{
	return _isAccept;
}

const bool DataInfo::readyAccept(SOCKET listenSocket, const unsigned __int64 curTime)
{
	_lastAcceptTime = curTime + 100;

	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_socket == INVALID_SOCKET)
		return false;

	ZeroMemory(&_accepOverlappedEx, sizeof(OverlappedInfo));

	DWORD bytes = 0;
	DWORD flags = 0;
	_accepOverlappedEx._wsaBuf.len = 0;
	_accepOverlappedEx._wsaBuf.buf = nullptr;
	_accepOverlappedEx._state = eIOState::ACCEPT;
	_accepOverlappedEx._sessionIndex = _Index;
	if (AcceptEx(listenSocket, _socket, _acceptBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED) & (_accepOverlappedEx)) == false)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
			return false;
	}

	return true;
}

const bool DataInfo::completeAccept()
{
	if (completeConnect(_iOCPHandle, _socket) == false)
		return false;

	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);
	char clientIP[32] = { 0, };
	inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);

	return true;
}

const bool DataInfo::completeConnect(HANDLE iocpHandle, SOCKET socket)
{
	_socket = socket;
	_isAccept = true;

	auto hIOCP = CreateIoCompletionPort((HANDLE)_socket, iocpHandle, (ULONG_PTR)(this), 0);
	if (hIOCP == INVALID_HANDLE_VALUE)
		return false;

	return bindRecieve();
}

void DataInfo::completeSend(const unsigned int size)
{
	std::lock_guard<std::mutex> guard(_lock);

	delete[] _sendDataList.front()->_wsaBuf.buf;
	delete _sendDataList.front();

	_sendDataList.pop();

	if (_sendDataList.empty() == false)
		sendIO();
}

const bool DataInfo::bindRecieve()
{
	DWORD flag = 0;
	DWORD recieveByte = 0;

	_recvOverlappedEx._wsaBuf.len = SOCK_BUF;
	_recvOverlappedEx._wsaBuf.buf = _recvBuf;
	_recvOverlappedEx._state = eIOState::RECV;

	const int rv = WSARecv(_socket,
		&(_recvOverlappedEx._wsaBuf),
		1,
		&recieveByte,
		&flag,
		(LPWSAOVERLAPPED) & (_recvOverlappedEx),
		NULL);

	if (rv == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		return false;

	return true;
}

// 1개의 스레드에서만 호출해야 한다!
const bool DataInfo::sendMessage(const unsigned int size, char* data)
{
	auto sendInfo = new OverlappedInfo;
	ZeroMemory(sendInfo, sizeof(OverlappedInfo));
	sendInfo->_wsaBuf.len = size;
	sendInfo->_wsaBuf.buf = new char[size];
	CopyMemory(sendInfo->_wsaBuf.buf, data, size);
	sendInfo->_state = eIOState::SEND;

	std::lock_guard<std::mutex> guard(_lock);

	_sendDataList.push(sendInfo);

	if (_sendDataList.size() >= 1)
		sendIO();

	return true;
}

const bool DataInfo::sendIO()
{
	auto sendOverlappedEx = _sendDataList.front();

	DWORD dwRecvNumBytes = 0;
	int rv = WSASend(_socket,
		&(sendOverlappedEx->_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)sendOverlappedEx,
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (rv == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		return false;

	return true;
}

void DataInfo::close(const bool isForce)
{
	struct linger stLinger = { 0, 0 };

	if (isForce == true)
		stLinger.l_onoff = 1;

	shutdown(_socket, SD_BOTH);
	setsockopt(_socket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	_isAccept = false;
	_lastAcceptTime = 0;

	closesocket(_socket);
	_socket = INVALID_SOCKET;
}

const const unsigned __int64 DataInfo::getLastAcceptTime()
{
	return _lastAcceptTime;
}

const unsigned int DataInfo::getIndex()
{
	return _Index;
}

char* DataInfo::getRecvBuffer()
{
	return _recvBuf;
}
