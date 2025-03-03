#pragma once
#include <functional>

//���庯��ָ��
//typedef int(*handleFunc)(void* arg);
//using handleFunc = int(*)(void*);

// �����ļ��������Ķ�д�¼�
enum class FDEvent
{
	TimeOut = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};

// �ɵ��ö����װ���������ʲô��1.����ָ�� 2.�ɵ��ö���(��������һ��ʹ�ã�
// ���յõ��˵�ַ������û�е���
class Channel
{
public:
	using handleFunc = std::function<int(void*)>;
	Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg);
	// �ص�����
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
	// �޸�fd��д�¼������ or ����⣩
	void writeEventEnable(bool flag);
	// �ж��Ƿ���Ҫ����ļ���������д�¼�
	bool isWriteEventEnable();
	// ȡ��˽�г�Ա��ֵ  ��������������Ҫѹջ��ֱ�ӽ��д�����滻�����Ч�ʣ���ռ�ø����ڴ�
	inline int getEvent()
	{
		return m_events;
	}
	inline int getSocket()
	{
		return m_fd;
	}
	inline const void* getArg()
	{
		return m_arg;
	}
private:
	// �ļ�������
	int m_fd;
	// �¼�
	int m_events;
	// �ص������Ĳ���
	void* m_arg;
};

