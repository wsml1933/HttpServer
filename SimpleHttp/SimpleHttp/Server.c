#include "Server.h"
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

struct FdInfo
{
	int fd;
	int epfd;
	pthread_t tid;
};


int initListenFd(unsigned short port)
{
	// 1.����������fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0); //����1��ѡipv4����ipv6������2��tcp����udp
	if (lfd == -1)
	{
		perror("socket");
		return -1;
	}
	// 2.���ö˿ڸ���
	// �������Ͽ����ӵ�һ����Ҫһ���Ӳ����ͷŰ󶨵Ķ˿ڣ����ö˿ڸ��ÿ��������ʱ����ʹ�ö˿�
	int opt = 1;// �����˿ڸ���
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt); 
	if (ret == -1)
	{
		perror("setsockopt");
		return -1;
	}
	// 3.��
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; // ipЭ��
	addr.sin_port = htons(port); // �󶨶˿�
	addr.sin_addr.s_addr = INADDR_ANY; // �󶨱�������ip��ַ��������
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1)
	{
		perror("bind");
		return -1;
	}
	// 4.���ü���
	ret = listen(lfd, 128); // ����2��������һ�����ܺͶ��ٿͻ��˽������ӣ��ں������Ϊ128
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	// ����fd
	return lfd;
}

int epollRun(int lfd)
{

	// 1.����epollʵ��
	int epfd = epoll_create(1); // �����������ˣ������㼴�ɣ���Ϊ���򴴽�ʧ��
	if (epfd == -1)
	{
		perror("epoll_create");
		return -1;
	}
	// 2.lfd �������������
	struct epoll_event ev;
	ev.data.fd = lfd; // ��¼��ǰҪ������fd 
	ev.events = EPOLLIN; // ί��epoll�����¼� EPOLLIN���¼�
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev); // ���
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	// 3.���
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event); // evs����Ĵ�С

	while (1)
	{
		int num = epoll_wait(epfd, evs, size, -1); // ����Ϊ-1��û���¼���һֱ���������¼��������򷵻��¼������ĸ���
		for (int i = 0; i < num; i++)
		{
			struct FdInfo* info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
			int fd = evs[i].data.fd;
			info->epfd = epfd;
			info->fd = fd;
			if (fd == lfd) // �������Ҫ������fd
			{
				// ���������� accept����ʱ�����������Ϊ�Ѿ��ҵ�Ҫ�����������ӵ�fd��accept����Ҫ����
				//acceptClient(lfd, epfd); // ���������ӣ������µõ���cfd��ӵ�epoll����
				
				pthread_create(&info->tid, NULL, acceptClient, info);
			}
			else // ���������Ҫ������fd����˵���ǽ���ͨ�ŵ�fd�����ͽ�������
			{
				// ��Ҫ�ǽ��ܶԶ˵�����
				//recvHttpRequest(fd, epfd);
				pthread_create(&info->tid, NULL, recvHttpRequest, info);

			}
		}
	}

	return 0;
}

//int acceptClient(int lfd, int epfd)
void* acceptClient(void* arg)
{
	struct FdInfo* info = (struct FdInfo*)arg;
	// 1. ��������
	int cfd = accept(info->fd, NULL, NULL);
	if (cfd == -1)
	{
		perror("accept");
		return NULL;
	}
	// 2. ���÷�����
	// ��Ե��ˮƽ�����б�Ե������Ч����ߣ�fdĬ����������Ҫ����Ϊ������
	int flag = fcntl(cfd, F_GETFL); // ��ȡfd������
	flag = O_NONBLOCK; // ���÷���������
	fcntl(cfd, F_SETFL, flag); // ���޸ĺ���������û�cfd

	// 3. cdf��ӵ�epoll��
	struct epoll_event ev;
	ev.data.fd = cfd; 
	ev.events = EPOLLIN | EPOLLET; // Ĭ��Ϊˮƽ���ñ�Ե����
	int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return NULL;
	}
	printf("acceptclient threadId: %ld\n", info->tid);
	free(info);
	return NULL;
}

//int recvHttpRequest(int cfd, int epfd)
void* recvHttpRequest(void* arg)
{
	printf("begin recv...\n");
	struct FdInfo* info = (struct FdInfo*)arg;
	int len = 0, totle = 0;
	char tmp[1024] = { 0 };
	char buf[4096] = { 0 };
	while ((len = recv(info->fd, tmp, sizeof tmp, 0)) > 0)
	{
		// ��Ҫ����ȡ�����к��������Դ�����泬���˲�ҪҲ����ν
		if (totle + len < sizeof buf)
		{
			memcpy(buf + totle, tmp, len);
		}
		totle += len;
	}
	// �ж������Ƿ������ϣ�������Ϻͽ���ʧ�ܶ�����-1����ͨ��errno���֣�
	if (len == -1 && errno == EAGAIN)
	{
		// ����������
		char* pt = strstr(buf, "\r\n");
		int reqLen = pt - buf;
		buf[reqLen] = '\0';
		parseRequestLine(buf, info->fd);
	}
	else if (len == 0)
	{
		// �ͻ��˶Ͽ�������
		epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
		close(info->fd);
	}
	else
	{
		perror("recv"); 
	}
	printf("recvMsg threadId: %ld\n", info->tid);
	free(info);
	return NULL;
}

int parseRequestLine(const char* line, int cfd)
{
	// ����������  get /xxx/pic.jpg http/1.1
	char method[12];
	char path[1024];
	sscanf(line, "%[^ ] %[^ ]", method, path);
	printf("method: %s, path: %s\n", method, path);
	// �ж��Ƿ���get����
	if (strcasecmp(method, "get") != 0) // strcasecmp���������ִ�Сд
	{
		return -1;
	}
	decodeMsg(path, path);
	// ����ͻ�������ľ�̬��Դ��Ŀ¼�����ļ���
	// ����·���л�Ϊ���ص����·��
	char* file = NULL;
	if (strcmp(path, "/") == 0)
	{
		file = "./";
	}
	else
	{
		file = path + 1;
	}
	// ��ȡ�ļ�����
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		// �ļ������� -- �ظ�404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		sendFile("404.html", cfd);
		return 0;
	}
	// �ж��ļ�����(·�������ļ���
	if (S_ISDIR(st.st_mode)) // 1Ŀ¼  0�ļ�
	{
		// �����Ŀ¼�е����ݷ��͸��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);
	}
	else
	{ 
		// ���ļ������ݷ��͸��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);
	}
	return 0;
}

int sendFile(const char* fileName, int cfd)
{
	// 1.���ļ�
	int fd = open(fileName, O_RDONLY);
	assert(fd > 0); // ���ԱȽ��Ͽ�����������ϳ���ҵ�
#if 0
	// ʹ��tcp����һ�η�һ�Σ��ڴ��к�����
	while (1)
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);
		if (len > 0)
		{
			send(cfd, buf, len, 0);
			// ����һ�£���ֹ��������ղ����������Զ˴�Ϣ���ᣨ��Ҫ��
			usleep(10);
		}
		else if (len == 0) // �ļ�����
		{
			break;
		}
		else
		{
			perror("read");
		}
	}
#else
	off_t offset = 0;
	int size = lseek(fd, 0, SEEK_END); // ��ȡ�ļ���С
	lseek(fd, 0, SEEK_SET); // ��ָ���ƻؿ�ͷ
	// �����ļ�
	while (offset < size)
	{
		int ret = sendfile(cfd, fd, &offset, size - offset);
		printf("ret value: %d\n", ret);
		if (ret == -1 && errno == EAGAIN)
		{
			printf("û����...\n");
		}
	}
#endif
	close(fd);
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	// ״̬��
	char buf[4096] = { 0 };
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
	// ��Ӧͷ
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);

	send(cfd, buf, strlen(buf), 0);
	return 0;
}

const char* getFileType(const char* name)
{
	// a.jpg a.mp4 a.html
	// ����������ҡ�.���ַ�, �粻���ڷ���NULL
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// ���ı�
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}

int sendDir(const char* dirName, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort);
	for (int i = 0; i < num; i++)
	{
		// ȡ���ļ��� namelist ָ�����һ��ָ������ struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		stat(subPath, &st);
		// �ж���Ŀ¼���Ƿ�Ŀ¼
		if (S_ISDIR(st.st_mode))
		{
			// a��ǩ <a href="">name</a>                  %s/����Ŀ¼����
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		send(cfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof(buf)); // ���������
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	free(namelist);
	return 0;
}

// ���ַ�ת��Ϊ������
int hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

// ����
// to �洢����֮�������, ��������, from�����������, �������
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		// isxdigit -> �ж��ַ��ǲ���16���Ƹ�ʽ, ȡֵ�� 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// ��16���Ƶ��� -> ʮ���� �������ֵ��ֵ�����ַ� int -> char
			// B2 == 178
			// ��3���ַ�, �����һ���ַ�, ����ַ�����ԭʼ����
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

			// ���� from[1] �� from[2] ����ڵ�ǰѭ�����Ѿ��������
			from += 2;
		}
		else
		{
			// �ַ�����, ��ֵ
			*to = *from;
		}

	}
	*to = '\0';
}