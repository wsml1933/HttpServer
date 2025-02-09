#pragma once
#include "Dispatcher.h"

extern struct Dispatcher EpollDispatcher; // 使用别的文件的全局变量
struct EventLoop
{
	Dispatcher* dispatcher;
	void* dispatcherData;
};