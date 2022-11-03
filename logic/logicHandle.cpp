#include "Logic.h"
#include "MLock.h"
#include "Crc32.h"
#include "Error.h"


bool LogicSocket::HandleTest_Head(pConnection pConn, MSG_Head* pmesghdr,
	HttpRequest* pHttpInfo)
{
	if (pHttpInfo == NULL)
	{
		return false;
	}

	Mlock lock(&pConn->logicMutex);

	//��¼Http������Ϣ
	HttpInfo_Record(pConn, pHttpInfo);

	return true;
}


bool LogicSocket::HandleTest_GET(pConnection pConn, MSG_Head* pmesghdr,
	HttpRequest* pHttpInfo)
{
	if (pHttpInfo == NULL)
	{
		return false;
	}

	Mlock lock(&pConn->logicMutex);

	//��¼Http������Ϣ
	HttpInfo_Record(pConn, pHttpInfo);
	
	string path = html_content + pHttpInfo->headinfo[URL];
	if (path[path.size() - 1] == '/')
	{
		path += "index.html";
	}

	//�ж��ļ��Ƿ����
	struct stat status;
	if (stat(path.c_str(), &status) == -1)
	{
		path = html_content + "/404.html";
	}
	else
	{
		//�ж��ļ��Ƿ�Ϊ�ļ���
		if ((status.st_mode & S_IFMT) == S_IFDIR)
		{
			path += "/index.html";
		}
	}

	//cerr << "Debug==>" << "URL:> " << path << endl;

	//��д�����ļ���Ϣ
	char* sendbuf = new char[m_MSG_Hlen + path.size() + 1];
	memcpy(sendbuf, pmesghdr, m_MSG_Hlen);
	strcpy(sendbuf + m_MSG_Hlen, path.c_str());
	sendbuf[m_MSG_Hlen + path.size()] = '\0';

	//�����ļ�
	MsgSend(sendbuf);

	return true;
}


void LogicSocket::HttpInfo_Record(pConnection cptr ,HttpRequest* phttprq)
{
	int log = open(httpinfo_file.c_str(), O_WRONLY | O_APPEND);

	if (log == -1)//��־��ʧ�ܴ���
	{
		log = open(httpinfo_file.c_str(), O_WRONLY | O_CREAT, 0600);

		if (log == -1)
		{
			Error_insert_File(LOG_ERR,
				"Record info:> open httpinfo_file failed, %m.", errno);
			return;
		}
	}

	//��¼�ͻ���ip��˿�
	struct sockaddr_in* sockinfo = (sockaddr_in*)&cptr->s_addr;

	HttpRequest_Body ip_port;
	ip_port.name = inet_ntoa(sockinfo->sin_addr);
	ip_port.value = to_string(ntohs(sockinfo->sin_port));

	phttprq->body.push_back(ip_port);

	string currTime = GetTime();

	string content = "\n/*---------" + currTime + "---------*/\n";
	write(log, content.c_str(), content.size());

	content = phttprq->headinfo[0] + " " + phttprq->headinfo[1]
		+ " " + phttprq->headinfo[2] + '\n';
	write(log, content.c_str(), content.size());

	for (int i = 0; i < phttprq->body.size(); i++)
	{
		content = phttprq->body[i].name + ": "
			+ phttprq->body[i].value + '\n';
		write(log, content.c_str(), content.size());
	}

	int er = close(log);
	if (er == -1)
	{
		Error_insert_File(LOG_ERR,
			"Record info:> close httpinfo_file failed, %m.", errno);
	}
}


bool LogicSocket::HandleTest_POST(pConnection pConn, MSG_Head* pmesghdr,
	HttpRequest* pHttpInfo)
{
	if (pHttpInfo == NULL)
	{
		return false;
	}

	Mlock lock(&pConn->logicMutex);

	//��¼Http������Ϣ
	HttpInfo_Record(pConn, pHttpInfo);

	return true;
}