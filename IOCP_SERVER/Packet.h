#pragma once

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>
#include <queue>
#include <thread>
const const unsigned int		SOCK_BUF = 256;
const const unsigned __int64	RE_USE_SESSION_WAIT_TIMESEC = 3;

enum eIOState
{
	ACCEPT = 0,
	RECV = 1,
	SEND = 2
};

struct OverlappedInfo
{
	WSAOVERLAPPED	_wsaOverlapped;
	WSABUF			_wsaBuf;
	eIOState		_state;
	unsigned int	_sessionIndex;
};

struct PacketInfo
{
	unsigned int		_index;
	unsigned short		_packetId;
	unsigned short		_dataSize;
	char* _data;
};

struct SerializePacketInfo
{
	std::queue<PacketInfo>		_packetList;
	std::thread::id				_threadId;
};


enum  PACKET_ID : unsigned short
{
	//SYSTEM
	REQ_USER_CONNECT = 11,
	REQ_USER_DISCONNECT = 12,
	REQ_END = 30,

	REQ_LOGIN,
	ACK_LOGIN,
};


//////////////////////////////////////////////////////////////////////////
#pragma pack(push,1)
class Packet_Header
{
public:
	Packet_Header(void)
		:_packetLength(0)
		, _packetId(0)
		, _type(0)
	{

	}
	~Packet_Header(void);

	unsigned short _packetLength;
	unsigned short _packetId;
	unsigned short _type;
};

class ReqLogin : public Packet_Header
{
public:
	void set(unsigned int userNo, unsigned int key)
	{
		_userNo = userNo;
		_key = key;
	}
	unsigned int _userNo;
	unsigned int _key;
};

#pragma pack(pop) 
