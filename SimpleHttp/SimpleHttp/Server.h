#pragma once
// ��ֹͷ�ļ����ظ�����   #pragma once

// ��ʼ�����ڼ������׽���
int initListenFd(unsigned short port);
// ����epoll
int epollRun(int lfd);
// �Ϳͻ��˽�������
//int acceptClient(int lfd, int epfd);
void* acceptClient(void* arg);
// ����http����
//int recvHttpRequest(int cfd, int epfd);
void* recvHttpRequest(void* arg);
// ����������
int parseRequestLine(const char* line, int cfd);
// �����ļ�
int sendFile(const char* fileName, int cfd);
// ������Ӧͷ��״̬��+��Ӧͷ��
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);
// ͨ���ļ������Ҷ�Ӧ��content-type
const char* getFileType(const char* name);
// ����Ŀ¼
int sendDir(const char* dirName, int cfd);
int hexToDec(char c);
void decodeMsg(char* to, char* from);