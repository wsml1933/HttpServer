// #define _GNU_SOURCE
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>

Buffer::Buffer(int size) :m_capacity(size)
{
	m_data = (char*)malloc(size);
	bzero(m_data, size);
}

Buffer::~Buffer()
{
	if (m_data != nullptr)
	{
		free(m_data);
	}
}

void Buffer::extendRoom(int size)
{
	// 1.�ڴ湻�� - ����Ҫ����
	if (writeableSize() >= size)
	{
		return;
	}
	// 2.�ڴ���Ҫ�ϲ��Ź��� - ����Ҫ����
	// ʣ��Ŀ�д���ڴ� + �Ѷ����ڴ� > size
	else if (m_readPos + writeableSize() >= size)
	{
		// �õ�δ�����ڴ��С
		int readable = readableSize();
		// �ƶ��ڴ�
		memcpy(m_data, m_data + m_readPos, readable);
		// ����λ��
		m_readPos = 0;
		m_writePos = readable;
	}
	// 3.�ڴ治���� - ����
	else
	{
		void* temp = realloc(m_data, m_capacity + size);
		if (temp == nullptr)
		{
			return; // ʧ��
		}
		memset((char*)temp + m_capacity, 0, size);
		// ��������
		m_data = static_cast<char*>(temp);
		m_capacity += size;
	}
}

int Buffer::appendString(const char* data, int size)
{
	if (data == nullptr || size <= 0)
	{
		return -1;
	}
	// ����
	extendRoom(size);
	// ���ݿ���
	memcpy(m_data + m_writePos, data, size);
	m_writePos += size;
	return 0;
}

int Buffer::appendString(const char* data)
{
	int size = strlen(data);
	int ret = appendString(data, size);
	return ret;
}

int Buffer::appendString(const string data)
{
	int ret = appendString(data.data());
	return ret;
}

int Buffer::socketRead(int fd)
{
	// read/recv/readv
	struct iovec vec[2];
	// ��ʼ������Ԫ��
	int writeable = writeableSize();
	vec[0].iov_base = m_data + m_writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960);
	vec[1].iov_base = tmpbuf;
	vec[1].iov_len = 40960;
	int result = readv(fd, vec, 2);
	if (result == -1)
	{
		return -1;
	}
	else if (result <= writeable)
	{
		m_writePos += result;
	}
	else
	{
		m_writePos = m_capacity;
		appendString(tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char* Buffer::findCRLF()
{
	// strstr --> ���ַ�����ƥ�����ַ���������\0������
	// memmem --> �����ݿ���ƥ�������ݿ飨��Ҫָ�����ݿ��С��
	// void* memmem(const void* haystack, size_t haystacklen,
	//		const void* needle, size_t needlelen);
	char* ptr = (char*)memmem(m_data + m_readPos, readableSize(), "\r\n", 2);
	return ptr;
}

int Buffer::sendData(int socket)
{
	// �ж���������
	int readable = readableSize();
	if (readable > 0)
	{
		int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
		if (count)
		{
			m_readPos += count;
			usleep(1);
		}
		return count;
	}
	return 0;
}
