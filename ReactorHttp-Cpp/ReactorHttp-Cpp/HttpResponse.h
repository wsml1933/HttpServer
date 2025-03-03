#pragma once
#include "Buffer.h"
#include <map>
#include <functional>
using namespace std;

// ����״̬��ö��
enum class StatusCode
{
	Unknown,
	OK = 200,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404 
};

// ����ṹ��
class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();
	// �����Ӧͷ
	void addHeader(const string key, const string value);
	// ��֯http��Ӧ��Ϣ
	void prepareMsg(Buffer* sendBuf, int socket);
	function<void(const string, Buffer*, int)> sendDataFunc;
	inline void setFileName(string name)
	{
		m_fileName = name;
	}
	inline void setStatusCode(StatusCode code)
	{
		m_statusCode = code;
	}
private:
	// ״̬�У�״̬�룬״̬����
	StatusCode m_statusCode;
	string m_fileName;
	// ��Ӧͷ - ��ֵ��
	map<string,string> m_headers;
	// ����״̬��������Ķ�Ӧ��ϵ
	const map<int, string> m_info = {
		{200, "OK"},
		{301, "MovedPermanently"},
		{302, "MovedTemporarily"},
		{400, "BadRequest"},
		{404, "NotFound"},
	};
};

