#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

class Buffer :public std::string
{
public:
	Buffer() :std::string() {}
	Buffer(size_t size) :std::string() { resize(size); }
	operator char* () { return (char*)c_str(); }
	operator char* () const { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
};

enum SockAttr {
	SOCK_ISSERVER = 1,//�Ƿ������ 1��ʾ�� 0��ʾ�ͻ���
	SOCK_ISBLOCK = 2,//�Ƿ����� 1��ʾ���� 0��ʾ������
};

class CSockParam {
public:
	CSockParam() {
		bzero(&addr_in, sizeof(addr_in));
		bzero(&addr_un, sizeof(addr_un));
		port = -1;
		attr = 0;
	}
	CSockParam(const Buffer& ip, short port, int attr) {
		this->ip = ip;
		this->port = port;
		this->attr = attr;
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = port;
		addr_in.sin_addr.s_addr = inet_addr(ip);
	}
	CSockParam(const Buffer& path, int attr) {
		ip = path;
		addr_un.sun_family = AF_UNIX;
		strcpy(addr_un.sun_path, path);
		this->attr = attr;
	}
	~CSockParam() {}
	CSockParam(const CSockParam& param) {
		ip = param.ip;
		port = param.port;
		attr = param.attr;
		memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
		memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
	}
public:
	CSockParam& operator=(const CSockParam& param) {
		if (this != &param) {
			ip = param.ip;
			port = param.port;
			attr = param.attr;
			memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
			memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
		}
		return *this;
	}
	sockaddr* addrin() { return (sockaddr*)&addr_in; }
	sockaddr* addrun() { return (sockaddr*)&addr_un; }
public:
	//��ַ
	sockaddr_in addr_in;
	sockaddr_un addr_un;
	//ip
	Buffer ip;
	//�˿�
	short port;
	//�ο�SockAttr
	int attr;
};

class CSocketBase
{
public:
	//������������
	virtual ~CSocketBase() {
		m_status = 3;
		if (m_socket != -1) {
			int fd = m_socket;
			m_socket = -1;
			close(fd);
		}
	}
public:
	//��ʼ�� ������ �׽��ִ�����bind��listen  �ͻ��� �׽��ִ���
	virtual int Init(const CSockParam& param) = 0;
	//���� ������ accept �ͻ��� connect  ����udp������Ժ���
	virtual int Link(CSocketBase** pClient = NULL) = 0;
	//��������
	virtual int Send(const Buffer& data) = 0;
	//��������
	virtual int Recv(Buffer& data) = 0;
	//�ر�����
	virtual int Close() = 0;
protected:
	//�׽�����������Ĭ����-1
	int m_socket;
	//״̬ 0��ʼ��δ��� 1��ʼ����� 2������� 3�Ѿ��ر�
	int m_status;
};