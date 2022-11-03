#ifndef Logic
#define Logic

#include "Globar.h"
#include "Socket.h"

//MSocket的子类:用于业务逻辑（各种消息触发的事件）处理
class LogicSocket : public MSocket
{
public:

	LogicSocket();
	virtual ~LogicSocket();

	//初始化函数
	virtual bool Initialize();

public:
	//*********************各种处理函数**********************
	
	//HEAD处理函数
	bool HandleTest_Head(pConnection pConn, MSG_Head* pmesghdr,
		HttpRequest* pHttpInfo);
	
	//GET处理函数
	bool HandleTest_GET(pConnection pConn, MSG_Head* pmesghdr,
		HttpRequest* pHttpInfo);

	//POST处理函数
	bool HandleTest_POST(pConnection pConn, MSG_Head* pmesghdr,
		HttpRequest* pHttpInfo);

	//********************************************************
public:

	//用于线程接收信号处理
	virtual void ThreadReceive_Proc(char* msgInfo);

	//用于发送心跳包
	void HeartbeatPKG_Send(MSG_Head* pmesghdr);

	//用于记录http请求信息
	void HttpInfo_Record(pConnection cptr, HttpRequest* phttprq);

private:

	//处理函数总数
	unsigned short m_commandQuantity;
};

#endif
