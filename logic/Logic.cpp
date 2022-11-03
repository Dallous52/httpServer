#include "Logic.h"
#include "Crc32.h"
#include "MLock.h"
#include "Error.h"


//�����Ա����ָ�루��������
typedef bool (LogicSocket::* Handler)(pConnection pConn, MSG_Head* pmesghdr,
	HttpRequest* pHttpInfo);

//������ָ����ֵ
static const Handler ProcFunc[] = {
	&LogicSocket::HandleTest_Head,
	&LogicSocket::HandleTest_GET,
	&LogicSocket::HandleTest_POST
};


LogicSocket::LogicSocket()
{
	m_commandQuantity = (unsigned short)(sizeof(ProcFunc) / sizeof(Handler));
}


//�����ʼ���������ø����ʼ������
bool LogicSocket::Initialize()
{
	return MSocket::Initialize();
}


void LogicSocket::ThreadReceive_Proc(char* msgInfo)
{
	MSG_Head* pmesghdr = (MSG_Head*)msgInfo;
	//ָ����Ϣͷָ��

	HttpRequest* pHttpInfo = pmesghdr->conn->Httprq;
	//ָ��http����ָ��
	
	//��ȡ������ָ����
	unsigned short msgCode = 0;

	while (msgCode < m_HTTP_Acitve.size()
		&& m_HTTP_Acitve[msgCode] != pHttpInfo->headinfo[ACTIVE])
	{
		msgCode++;
	}

	pConnection pConn = pmesghdr->conn;

	//�ж������Ƿ����
	if (pConn->serialNum != pmesghdr->serialNum)
	{
		//�ͻ����쳣�Ͽ����������ð�
		return;
	}

	//�ж��û����͵���Ϣ���Ƿ���ȷ
	if (msgCode + 1 >= m_commandQuantity)
	{
		//cerr << "Message code is out of bounds, package discard." << endl;
		return;
	}

	//ִ����Ϣ���Ӧ�Ĵ�����
	(this->*ProcFunc[msgCode + 1])(pConn, pmesghdr, pHttpInfo);

	return;
}


void LogicSocket::HeartbeatPKG_Send(MSG_Head* pmesghdr)
{
	//char* buffer = new char[m_MSG_Hlen + m_PKG_Hlen];
	//char* auxbuf = buffer;

	////������Ϣͷ
	//memcpy(auxbuf, pmesghdr, m_MSG_Hlen);
	//auxbuf = auxbuf + m_MSG_Hlen;

	////�����ͷ
	//PKG_Head* pkghdr = (PKG_Head*)auxbuf;
	//pkghdr->mesgCode = htons(0);
	//pkghdr->pkgLen = htons(m_PKG_Hlen);
	//pkghdr->crc32 = 0;

	////������Ϣ
	//MsgSend(buffer);

	return;
}

LogicSocket::~LogicSocket()
{

}