#ifndef Logic
#define Logic

#include "Globar.h"
#include "Socket.h"

//MSocket������:����ҵ���߼���������Ϣ�������¼�������
class LogicSocket : public MSocket
{
public:

	LogicSocket();
	virtual ~LogicSocket();

	//��ʼ������
	virtual bool Initialize();

public:
	//*********************���ִ�����**********************
	
	//HEAD������
	bool HandleTest_Head(pConnection pConn, MSG_Head* pmesghdr,
		HttpRequest* pHttpInfo);
	
	//GET������
	bool HandleTest_GET(pConnection pConn, MSG_Head* pmesghdr,
		HttpRequest* pHttpInfo);

	//POST������
	bool HandleTest_POST(pConnection pConn, MSG_Head* pmesghdr,
		HttpRequest* pHttpInfo);

	//********************************************************
public:

	//�����߳̽����źŴ���
	virtual void ThreadReceive_Proc(char* msgInfo);

	//���ڷ���������
	void HeartbeatPKG_Send(MSG_Head* pmesghdr);

	//���ڼ�¼http������Ϣ
	void HttpInfo_Record(pConnection cptr, HttpRequest* phttprq);

private:

	//����������
	unsigned short m_commandQuantity;
};

#endif
