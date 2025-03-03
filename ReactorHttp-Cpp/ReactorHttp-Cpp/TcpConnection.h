#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

//#define MSG_SEND_AUTO 
class TcpConnection
{
public:
	TcpConnection(int fd, struct EventLoop* evloop);
	~TcpConnection();
	static int processRead(void* arg);
	static int processWrite(void* arg);
	static int destroy(void* arg);
private:
	EventLoop* m_evLoop;
	Channel* m_channel;
	Buffer* m_readBuf;
	Buffer* m_writeBuf;
	string m_name;
	// http –≠“È
	HttpRequest* m_request;
	HttpResponse* m_response;
};