#include "Logic.h"
#include "Crc32.h"
#include "MLock.h"
#include "Error.h"


//定义成员函数指针（处理函数）
typedef bool (LogicSocket::* Handler)(pConnection pConn, MSG_Head* pmesghdr,
	HttpRequest* pHttpInfo);

//处理函数指针数值
static const Handler ProcFunc[] = {
	&LogicSocket::HandleTest_Head,
	&LogicSocket::HandleTest_GET,
	&LogicSocket::HandleTest_POST
};


LogicSocket::LogicSocket()
{
	m_commandQuantity = (unsigned short)(sizeof(ProcFunc) / sizeof(Handler));
}


//子类初始化函数调用父类初始化函数
bool LogicSocket::Initialize()
{
	return MSocket::Initialize();
}


void LogicSocket::ThreadReceive_Proc(char* msgInfo)
{
	MSG_Head* pmesghdr = (MSG_Head*)msgInfo;
	//指向消息头指针

	HttpRequest* pHttpInfo = pmesghdr->conn->Httprq;
	//指向http请求指针
	
	//获取处理函数指令码
	unsigned short msgCode = 0;

	while (msgCode < m_HTTP_Acitve.size()
		&& m_HTTP_Acitve[msgCode] != pHttpInfo->headinfo[ACTIVE])
	{
		msgCode++;
	}

	pConnection pConn = pmesghdr->conn;

	//判断连接是否过期
	if (pConn->serialNum != pmesghdr->serialNum)
	{
		//客户端异常断开，丢弃无用包
		return;
	}

	//判断用户发送的消息码是否正确
	if (msgCode + 1 >= m_commandQuantity)
	{
		//cerr << "Message code is out of bounds, package discard." << endl;
		return;
	}

	//执行消息码对应的处理函数
	(this->*ProcFunc[msgCode + 1])(pConn, pmesghdr, pHttpInfo);

	return;
}


void LogicSocket::HeartbeatPKG_Send(MSG_Head* pmesghdr)
{
	//char* buffer = new char[m_MSG_Hlen + m_PKG_Hlen];
	//char* auxbuf = buffer;

	////填入消息头
	//memcpy(auxbuf, pmesghdr, m_MSG_Hlen);
	//auxbuf = auxbuf + m_MSG_Hlen;

	////填入包头
	//PKG_Head* pkghdr = (PKG_Head*)auxbuf;
	//pkghdr->mesgCode = htons(0);
	//pkghdr->pkgLen = htons(m_PKG_Hlen);
	//pkghdr->crc32 = 0;

	////返送消息
	//MsgSend(buffer);

	return;
}

LogicSocket::~LogicSocket()
{

}