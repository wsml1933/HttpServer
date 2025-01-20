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
	// 1.创建监听的fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0); //参数1：选ipv4还是ipv6；参数2：tcp还是udp
	if (lfd == -1)
	{
		perror("socket");
		return -1;
	}
	// 2.设置端口复用
	// 当主动断开连接的一方需要一分钟才能释放绑定的端口，设置端口复用可以在这个时间内使用端口
	int opt = 1;// 开启端口复用
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt); 
	if (ret == -1)
	{
		perror("setsockopt");
		return -1;
	}
	// 3.绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; // ip协议
	addr.sin_port = htons(port); // 绑定端口
	addr.sin_addr.s_addr = INADDR_ANY; // 绑定本地任意ip地址（网卡）
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1)
	{
		perror("bind");
		return -1;
	}
	// 4.设置监听
	ret = listen(lfd, 128); // 参数2：监听中一次性能和多少客户端建立连接，内核中最大为128
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	// 返回fd
	return lfd;
}

int epollRun(int lfd)
{

	// 1.创建epoll实例
	int epfd = epoll_create(1); // 参数被弃用了，大于零即可，若为零则创建失败
	if (epfd == -1)
	{
		perror("epoll_create");
		return -1;
	}
	// 2.lfd 上树（红黑树）
	struct epoll_event ev;
	ev.data.fd = lfd; // 记录当前要操作的fd 
	ev.events = EPOLLIN; // 委托epoll检测的事件 EPOLLIN读事件
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev); // 添加
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	// 3.检测
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event); // evs数组的大小

	while (1)
	{
		int num = epoll_wait(epfd, evs, size, -1); // 设置为-1，没有事件则一直堵塞，有事件触发了则返回事件触发的个数
		for (int i = 0; i < num; i++)
		{
			struct FdInfo* info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
			int fd = evs[i].data.fd;
			info->epfd = epfd;
			info->fd = fd;
			if (fd == lfd) // 如果是我要监听的fd
			{
				// 建立新连接 accept，此时不会堵塞，因为已经找到要建立的新连接的fd，accept不需要阻塞
				//acceptClient(lfd, epfd); // 建立新连接，并把新得到的cfd添加到epoll树上
				
				pthread_create(&info->tid, NULL, acceptClient, info);
			}
			else // 如果不是我要监听的fd，则说明是进行通信的fd，发送接收数据
			{
				// 主要是接受对端的数据
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
	// 1. 建立连接
	int cfd = accept(info->fd, NULL, NULL);
	if (cfd == -1)
	{
		perror("accept");
		return NULL;
	}
	// 2. 设置非阻塞
	// 边缘和水平，其中边缘非阻塞效率最高，fd默认阻塞，需要设置为非阻塞
	int flag = fcntl(cfd, F_GETFL); // 获取fd的属性
	flag = O_NONBLOCK; // 设置非阻塞属性
	fcntl(cfd, F_SETFL, flag); // 将修改后的属性设置回cfd

	// 3. cdf添加到epoll中
	struct epoll_event ev;
	ev.data.fd = cfd; 
	ev.events = EPOLLIN | EPOLLET; // 默认为水平设置边缘触发
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
		// 主要是提取请求行和请求的资源，后面超过了不要也无所谓
		if (totle + len < sizeof buf)
		{
			memcpy(buf + totle, tmp, len);
		}
		totle += len;
	}
	// 判断数据是否接受完毕（接收完毕和接受失败都返回-1可以通过errno区分）
	if (len == -1 && errno == EAGAIN)
	{
		// 解析请求行
		char* pt = strstr(buf, "\r\n");
		int reqLen = pt - buf;
		buf[reqLen] = '\0';
		parseRequestLine(buf, info->fd);
	}
	else if (len == 0)
	{
		// 客户端断开了连接
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
	// 解析请求行  get /xxx/pic.jpg http/1.1
	char method[12];
	char path[1024];
	sscanf(line, "%[^ ] %[^ ]", method, path);
	printf("method: %s, path: %s\n", method, path);
	// 判断是否是get请求
	if (strcasecmp(method, "get") != 0) // strcasecmp（）不区分大小写
	{
		return -1;
	}
	decodeMsg(path, path);
	// 处理客户端请求的静态资源（目录或者文件）
	// 将其路径切换为本地的相对路径
	char* file = NULL;
	if (strcmp(path, "/") == 0)
	{
		file = "./";
	}
	else
	{
		file = path + 1;
	}
	// 获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		// 文件不存在 -- 回复404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		sendFile("404.html", cfd);
		return 0;
	}
	// 判断文件类型(路径还是文件）
	if (S_ISDIR(st.st_mode)) // 1目录  0文件
	{
		// 把这个目录中的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);
	}
	else
	{ 
		// 把文件的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);
	}
	return 0;
}

int sendFile(const char* fileName, int cfd)
{
	// 1.打开文件
	int fd = open(fileName, O_RDONLY);
	assert(fd > 0); // 断言比较严苛，如果不符合程序挂掉
#if 0
	// 使用tcp，读一段发一段，内存有好申请
	while (1)
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);
		if (len > 0)
		{
			send(cfd, buf, len, 0);
			// 休眠一下，防止浏览器接收不过来，给对端喘息机会（重要）
			usleep(10);
		}
		else if (len == 0) // 文件读完
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
	int size = lseek(fd, 0, SEEK_END); // 获取文件大小
	lseek(fd, 0, SEEK_SET); // 将指针移回开头
	// 读大文件
	while (offset < size)
	{
		int ret = sendfile(cfd, fd, &offset, size - offset);
		printf("ret value: %d\n", ret);
		if (ret == -1 && errno == EAGAIN)
		{
			printf("没数据...\n");
		}
	}
#endif
	close(fd);
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	// 状态行
	char buf[4096] = { 0 };
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
	// 响应头
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);

	send(cfd, buf, strlen(buf), 0);
	return 0;
}

const char* getFileType(const char* name)
{
	// a.jpg a.mp4 a.html
	// 自右向左查找‘.’字符, 如不存在返回NULL
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// 纯文本
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
		// 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		stat(subPath, &st);
		// 判断是目录还是非目录
		if (S_ISDIR(st.st_mode))
		{
			// a标签 <a href="">name</a>                  %s/代表目录里面
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
		memset(buf, 0, sizeof(buf)); // 发送完清空
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	free(namelist);
	return 0;
}

// 将字符转换为整形数
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

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2;
		}
		else
		{
			// 字符拷贝, 赋值
			*to = *from;
		}

	}
	*to = '\0';
}