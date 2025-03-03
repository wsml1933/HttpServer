#include "TcpConnection.h"
#include "HttpRequest.h"
#include <stdlib.h>
#include <stdio.h>
#include "Buffer.h"
#include "Log.h"

int TcpConnection::processRead(void* arg)
{
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// ��������
	int socket = conn->m_channel->getSocket();
	int count = conn->m_readBuf->socketRead(socket);
	Debug("���յ���http�������ݣ�%s", conn->m_readBuf->data());
	if (count > 0)
	{
		// ������http���󣬽���http����
#ifdef MSG_SEND_AUTO
		conn->m_channel->writeEventEnable(true);
		conn->m_evLoop->addTask(conn->m_channel, ElementType::MODIFY);
#endif
		bool flag = conn->m_request->parseHttpRequest(
			conn->m_readBuf, conn->m_response,
			conn->m_writeBuf, socket);
		if (!flag)
		{
			// ����ʧ�ܣ��ظ�һ���򵥵�html
			string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_writeBuf->appendString(errMsg);
		}
	}
	else
	{
#ifdef MSG_SEND_AUTO
		// �Ͽ�����
		conn->m_evLoop->addTask(conn->m_channel, ElementType::DELETE);
#endif
	}
#ifndef MSG_SEND_AUTO
	// �Ͽ�����
	conn->m_evLoop->addTask(conn->m_channel, ElementType::DELETE);
#endif
	return 0;
}

int TcpConnection::processWrite(void* arg)
{
	Debug("��ʼ���������ˣ�����д�¼����ͣ�....");
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// ��������
	int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
	if (count > 0)
	{
		// �ж������Ƿ�ȫ�����ͳ�ȥ��
		if (conn->m_writeBuf->readableSize() == 0)
		{
			// 1.�����д�¼� -- �޸�channel�б�����¼�
			conn->m_channel->writeEventEnable(false);
			// 2.�޸�dispatcher��⼯�� -- �������ڵ�
			conn->m_evLoop->addTask(conn->m_channel, ElementType::MODIFY);
			// 3.ɾ������ڵ�
			conn->m_evLoop->addTask(conn->m_channel, ElementType::DELETE);
		}
	}
	return 0;
}

int TcpConnection::destroy(void* arg)
{
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	if (conn != nullptr)
	{
		delete conn;
	}
	return 0;
}

TcpConnection::TcpConnection(int fd, EventLoop* evloop)
{
	m_evLoop = evloop;
	m_readBuf = new Buffer(10240);
	m_writeBuf = new Buffer(10240);
	// http
	m_request = new HttpRequest;
	m_response = new HttpResponse;
	m_name = "Connection-" + to_string(fd);
	m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, processWrite, destroy, this);
	evloop->addTask(m_channel, ElementType::ADD);
}

TcpConnection::~TcpConnection()
{
	if (m_readBuf && m_readBuf->readableSize() == 0 &&
		m_writeBuf && m_writeBuf->readableSize() == 0)
	{
		delete m_readBuf;
		delete m_writeBuf;
		m_evLoop->freeChannel(m_channel);
		delete m_request;
		delete m_response;
	}
	//Debug("���ӶϿ����ͷ���Դ��gameover,connName: %s", m_name);
}
