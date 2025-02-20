#include "EventLoop.h"
#include <assert.h>

struct EventLoop* eventLoopInit()
{
    return eventLoopInitEx(NULL);
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
    struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
    evLoop->isQuit = false;
    evLoop->threadID = pthread_self();
    pthread_mutex_init(&evLoop->mutex, NULL);
    strcpy(evLoop->threadName, threadName == NULL ? "MainThread" : threadName);
    evLoop->dispatcher = &EpollDispatcher;
    evLoop->dispatcherData = evLoop->dispatcher->init();
    // ����
    evLoop->head = evLoop->tail = NULL;
    // map
    evLoop->channelMap = channelMapInit(128);
    return evLoop;
}

int eventLoopRun(struct EventLoop* evLoop)
{
    assert(evLoop != NULL);
    // ȡ���¼��ַ��ͼ��ģ��
    struct Dispatcher* dispatcher = evLoop->dispatcher;
    // �Ƚ��߳�ID�Ƿ�����
    if (evLoop->threadID != pthread_self())
    {
        return -1;
    }
    // ѭ�������¼�����
    while (!evLoop->isQuit)
    {
        dispatcher->dispatch(evLoop, 2); // ��ʱʱ�� 2s
    }
    return 0;
}

int eventActivate(struct EventLoop* evLoop, int fd, int event)
{
    if (fd < 0 || evLoop == NULL)
    {
        return -1;
    }
    // ȡ��channel
    struct Channel* channel = evLoop->channelMap->list[fd];
    assert(channel->fd == fd);
    if (event & ReadEvent && channel->readCallback) // ���ǻص�Ϊ����ִ����
    {
        channel->readCallback(channel->arg);
    }
    if (event & WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(channel->arg);
    }
    return 0;
}

int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type)
{
    // ����������������Դ
    pthread_mutex_lock(&evLoop->mutex);
    // �����½ڵ�
    struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    // ����Ϊ��
    if (evLoop->head == NULL)
    {
        evLoop->head = evLoop->tail = node;
    }
    else
    {
        evLoop->tail->next = node;
        evLoop->tail = node;
    }
    pthread_mutex_unlock(&evLoop->mutex);
    // ����ڵ�
    /*
    * ϸ�ڣ�
    *   1. ��������ڵ����ӣ������ǵ�ǰ�߳�Ҳ�����������̣߳����̣߳�
    *       1��. �޸�fd���¼�����ǰ���̷߳��𣬵�ǰ���̴߳���
    *       2��. ����µ�fd���������ڵ�Ĳ����������̷߳����
    *   2. ���������̴߳���������У���Ҫ�ɵ�ǰ�����߳�ȥ����
    */
    if (evLoop->threadID == pthread_self())
    {
        // ��ǰ���߳�
    }
    else
    {
        // ���߳� -- �������̴߳�����������е�����
        // 1.���߳��ڹ��� 2.���̱߳������ˣ�select��poll��epoll     ��evloop�����һ�����ڽ��������fd���Լ����޸�ʹ��������
    }

    return 0;
}
