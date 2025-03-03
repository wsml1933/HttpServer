#include "HttpResponse.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#define ResHeaderSize 16

void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
	
}

HttpResponse::HttpResponse()
{
	m_statusCode = StatusCode::Unknown;
	m_headers.clear();
	m_fileName = string();
	sendDataFunc = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const string key, const string value)
{
	if (key.empty() || value.empty())
	{
		return;
	}
	m_headers.insert(make_pair(key, value));
}

void HttpResponse::prepareMsg(Buffer* sendBuf, int socket)
{
	// ״̬��
	char tmp[1024] = { 0 };
	int code = static_cast<int>(m_statusCode);
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, m_info.at(code).data());
	sendBuf->appendString(tmp);
	// ��Ӧͷ
	for (auto it = m_headers.begin(); it != m_headers.end(); it++)
	{
		sprintf(tmp, "%s: %s\r\n", it->first.data(), it->second.data());
		sendBuf->appendString(tmp);
	}
	// ����
	sendBuf->appendString("\r\n");
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(socket);
#endif 

	// �ظ�������
	sendDataFunc(m_fileName, sendBuf, socket);
}
