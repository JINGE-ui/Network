#pragma once
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <io.h>
#include <Mswsock.h>
#pragma comment(lib, "Mswsock.lib")
using namespace std;
#pragma comment(lib,"ws2_32.lib")
//从报文请求分离出文件名
void split_filename(char* revbuf,int buflen,char* file_name) {
	int flag = 0;
	memset(file_name, 500, 0);
	int k = 0;
	for (int i = 0; i < buflen; i++) {
		if (revbuf[i] == ' ') {
			if (revbuf[i + 1] == '/') {
				flag = 1;
				i++;
				continue;
			}
			else {
				flag = 0;
				break;
			}
		}
		if (flag) {
			file_name[k++] = revbuf[i];
		}
	}
	file_name[k] = 0;
}

//定义文件类型对应的content-tyoe
struct doc_type {
	char* suffix;
	char* type;
};
//文件
struct doc_type file_type[] =
{
	{"html",  "text/html"},
	{"gif",   "imag/gif"},
	{"jpg",  "imag/jpeg"},
	{NULL,    NULL}
};

//响应首部内容
char* http_res_hdr_tmp1 = "HTTP/1.1 200 OK \r\n\Content-Length:%d\r\nConnection:close\r\nContent-Type:%s\r\n\r\n";
char* http_404 = "HTTP/1.1 404 Not Found \r\n\Content-Length:%d\r\nConnection:close\r\nContent-Type:%s\r\n\r\n";

//通过后缀，查找到对应的content-type
char* http_get_type_by_suffix(const char* suffix)
{
	struct doc_type* type;
	for (type = file_type; type->suffix; type++)
	{
		if (strcmp(type->suffix, suffix) == 0)
			return type->type;
	}
	return NULL;
}

//解析客户端发送过来的请求
int http_send_response(SOCKET soc, char* buf, int buf_len,char* file_name)
{
	int read_len, file_len, hdr_len, send_len;
	char* type;
	char read_buf[1024];
	char http_header[1024];
	char suffix[15];
	FILE* res_file;
	//获取suffix
	int k = 0;
	int len = strlen(file_name);
	for (int i = 0,flag = 0; i < strlen(file_name); i++) {
		if (file_name[i] == '.') {
			flag = 1;
			continue;
		}
		if (flag) {
			suffix[k++] = file_name[i];
		}
	}
	suffix[k] = 0;
	char file_read_name[1024];
	char http_condition[1024];
	strcpy(http_condition, http_res_hdr_tmp1);
	strcpy(file_read_name, file_name);
	//获得文件content-type
	type = http_get_type_by_suffix(suffix);
	if (type == NULL)
	{
		if (suffix == "iso") {
			return 0;
		}
		if (!strlen(file_name)) {
			strcpy(file_read_name, "homepage.html");
		}
		else {
			printf("error:服务器没有相关的文件类型!\n");
			strcpy(file_read_name, "error.html");
			strcpy(http_condition, http_404);
		}
	}
	//打开文件
	res_file = fopen(file_read_name, "rb+");
	if (res_file == NULL)
	{
		printf("服务器文件:%s 不存在!\n", file_name);
		res_file = fopen("error.html", "rb+");
		strcpy(http_condition, http_404);
	}
	//计算文件大小
	fseek(res_file, 0, SEEK_END);
	file_len = ftell(res_file);
	fseek(res_file, 0, SEEK_SET);

	//构造响应首部，加入文件长度，content-type信息
	hdr_len = sprintf(http_header, http_condition, file_len, type);
	send_len = send(soc, http_header, hdr_len, 0);
	printf("服务器发送的响应头部是:\n%s\n", http_header);
	if (send_len == SOCKET_ERROR)
	{
		fclose(res_file);
		printf("[Web]发送失败，错误:%d\n", WSAGetLastError());
		return 0;
	}

	//发送文件
	do
	{
		read_len = fread(read_buf, sizeof(char), 1024, res_file);
		if (read_len > 0)
		{
			send_len = send(soc, read_buf, read_len, 0);
			file_len -= read_len;
		}
	} while ((read_len > 0) && (file_len > 0));
	fclose(res_file);
	return 1;
}
void main() {
	WSADATA wsaData;
	fd_set rfds;				//用于检查socket是否有数据到来的的文件描述符，用于socket非阻塞模式下等待网络事件通知（有数据到来）
	fd_set wfds;				//用于检查socket是否可以发送的文件描述符，用于socket非阻塞模式下等待网络事件通知（可以发送数据）
	bool first_connetion = true;

	int nRc = WSAStartup(0x0202, &wsaData);

	if (nRc) {
		printf("Winsock  startup failed with error!\n");
	}

	if (wsaData.wVersion != 0x0202) {
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock  startup Ok!\n");

	SOCKET srvSocket;
	sockaddr_in addr, clientAddr;
	SOCKET sessionSocket;
	int addrLen;
	//create socket
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");
	//set port and ip
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5050);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);  //inet_addr("1.2.3.4");
	//binding
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");
	//listen
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n\n\n");

	clientAddr.sin_family = AF_INET;
	addrLen = sizeof(clientAddr);
	char recvBuf[4096];

	//设置等待客户连接请求
	while (true) {
		//产生会话SOCKET
		sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
		if (sessionSocket != INVALID_SOCKET)
			printf("Socket listen one client request!\n");

		//receiving data from client
		memset(recvBuf, '\0', 4096);
		rtn = recv(sessionSocket, recvBuf, 256, 0);
		if (rtn > 0) {
			printf("Received %d bytes from client: \n%s\n\n", rtn, recvBuf);
		}
		char file_name[1024];
		memset(file_name, 0, 1024);
		split_filename(recvBuf, rtn, file_name);
		int nRC = http_send_response(sessionSocket, recvBuf, rtn, file_name);
		if (nRC == SOCKET_ERROR) {
			printf("send error!\n");
		}
	}
	closesocket(srvSocket);
	WSACleanup();
}