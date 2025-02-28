#pragma once

struct Buffer
{
	// ָ���ڴ��ָ��
	char* data;
	int capacity;
	int readPos;
	int writePos;
};

// ��ʼ��
struct Buffer* bufferInit(int size);
// �����ڴ�
void bufferDestroy(struct Buffer* buf);
// ����
void bufferExtendRoom(struct Buffer* buffer, int size);
// �õ�ʣ��Ŀ�д���ڴ�����
int bufferWriteableSize(struct Buffer* buffer);
// �õ�ʣ��Ŀɶ����ڴ�����
int bufferReadableSize(struct Buffer* buffer);
// д�ڴ� 1.ֱ��д 2. �����׽�������
int bufferAppendData(struct Buffer* buffer, const char* data, int size);
int bufferAppendString(struct Buffer* buffer, const char* data);
int bufferSocketRead(struct Buffer* buffer, int fd);
// ����\r\nȡ��һ�У��ҵ��������ݿ��е�λ�ã����ظ�λ��
char* bufferFindCRLF(struct Buffer* buffer);
// ��������
int bufferSendData(struct Buffer* buffer, int socket);