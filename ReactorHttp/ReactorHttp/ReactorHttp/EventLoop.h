#pragma once
#include <stdbool.h>
#include <pthread.h>
#include "Dispatcher.h"
#include "ChannelMap.h"

extern struct Dispatcher EpollDispatcher; // ʹ�ñ���ļ���ȫ�ֱ���
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;

// ����ýڵ��е�channel�ķ�ʽ
enum ElementType{ADD, DELECT, MODIFY};

// ��������ڵ�
struct ChannelElement
{
	int type; // ��δ���ýڵ��е�channel
	struct Channel* channel;
	struct ChannelElement* next;
};
struct EventLoop
{
	bool isQuit;
	struct Dispatcher* dispatcher;
	void* dispatcherData;
	// �������
	struct ChannelElement* head;
	struct ChannelElement* tail;
	// map
	struct ChannelMap* channelMap;
	// �߳�id��name��mutex
	pthread_t threadID;
	char threadName[32];
	pthread_mutex_t mutex;
};

// ��ʼ��
struct EventLoop* eventLoopInit(); // ���߳�
struct EventLoop* eventLoopInitEx(const char* threadName); // ���߳�
// ������Ӧ��ģ��
int eventLoopRun(struct EventLoop* evLoop);
// �������ļ�fd�ĺ���
int eventActivate(struct EventLoop* evLoop, int fd, int event);
// ��������������
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type);