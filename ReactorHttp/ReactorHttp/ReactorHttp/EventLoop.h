#pragma once
#include "Dispatcher.h"

extern struct Dispatcher EpollDispatcher; // ʹ�ñ���ļ���ȫ�ֱ���
struct EventLoop
{
	Dispatcher* dispatcher;
	void* dispatcherData;
};