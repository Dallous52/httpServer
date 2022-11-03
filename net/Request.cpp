#include "Socket.h"
#include "Manage.h"
#include "Error.h"
#include "Process.h"
#include "MLock.h"

void MSocket::Read_requestHandle(pConnection cptr)
{
	ssize_t recpro = ReceiveProc(cptr);

	bool isFlood = false;

	if (recpro <= 0)
	{
		return;
	}

	if (cptr->recvStat == _PKG_HD_INIT)
	{
		//http���н������������д���
		Request_Handle_A(cptr, isFlood);
	}
	else if (cptr->recvStat == _PKG_BD_INIT)
	{
		string tmpstr = cptr->bufptr;
		//http�ֶμ����ݽ���
		size_t semi = tmpstr.find(':');

		if (semi != string::npos)
		{
			HttpRequest_Body tmp;

			//cerr << "size:>" << recpro << " | " << semi << endl;
			tmp.name.append(tmpstr, 0, semi);
			tmp.value.append(tmpstr, semi + 2, recpro - semi - 3);

			cptr->Httprq->body.push_back(tmp);
			memset(cptr->bufptr, 0, _LINE_MAX_LENGTH);
		}
		else
		{
			if (m_floodCheckStart)
			{
				isFlood = TestFlood(cptr);
			}
			Request_Handle_L(cptr, isFlood);
		}
	}

	if (isFlood)
	{
		Error_insert_File(LOG_CRIT, "Flooding attack detected, close connection");
		ActiveShutdown(cptr);
	}

	return;
}


ssize_t MSocket::ReceiveProc(pConnection cptr)
{
	size_t receive = 0;
	char aux = 0;

	while (receive < _LINE_MAX_LENGTH && aux != '\n')
	{
		uint16_t n = recv(cptr->fd, &aux, 1, 0);
		//recvϵͳ�������ڽ���socket����

		if (n == 0)
		{
			//�ͻ��������رգ���������
			Error_insert_File(LOG_NOTICE, "The client shuts down normally.");

			ActiveShutdown(cptr);

			return -1;
		}

		//������
		if (n < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				//��ETģʽ�¿��ܳ��֣���ʾû���յ�����
				Error_insert_File(LOG_WARN, "receive data:> no data in Socket buffer.");
				return -1;
			}

			if (errno == EINTR)
			{
				Error_insert_File(LOG_WARN, "receive data:> interrupted system call.");
				return -1;
			}

			if (errno == ECONNRESET)
			{
				//�Եȷ���������
				//���û�ǿ�ƹرյ��µ��������
				//do nothing
			}
			else
			{
				Error_insert_File(LOG_SERIOUS, "receive data:> %m", errno);
			}

			Error_insert_File(LOG_NOTICE, "The client abnormal shutdow.");

			ActiveShutdown(cptr);

			return -1;
		}//end if

		if (aux == '\r')
		{
			n = recv(cptr->fd, &aux, 1, MSG_PEEK);
			if (n > 0 && aux == '\n')
			{
				recv(cptr->fd, &aux, 1, 0);
			}
			else
			{
				aux = '\n';
			}
		}

		cptr->bufptr[receive++] = aux;
	}

	cptr->bufptr[receive] = '\0';

	return receive;
}


void MSocket::Request_Handle_A(pConnection cptr, bool& isFlood)
{

	//�����/���������
	if (IsMalicious(cptr->bufptr))
	{
		cptr->recvStat = _PKG_HD_INIT;
		memset(cptr->bufptr, 0, _LINE_MAX_LENGTH);
	}
	else
	{
		//�ԺϷ���ͷ���д���

		cptr->isNewRecv = true;

		string tmpstr = cptr->bufptr;

		//��дhttpͷ
		for (int i = 0; i < 3; i++)
		{
			size_t mid = tmpstr.find(' ');

			if (mid != string::npos)
			{
				cptr->Httprq->headinfo[i].append(tmpstr, 0, mid);
				tmpstr.erase(0, mid + 1);
			}
			else
			{
				mid = tmpstr.find('\n');

				if (mid != string::npos)
				{
					cptr->Httprq->headinfo[i].append(tmpstr, 0, mid);
					tmpstr.erase(0, mid);

					break;
				}
			}
		}
	
		//��ʼ�������
		cptr->recvStat = _PKG_BD_INIT;
		memset(cptr->bufptr, 0, _LINE_MAX_LENGTH);
		
	}//end if

	return;
}


bool MSocket::IsMalicious(const string buffer)
{
	for (int i = 0; i < m_HTTP_Acitve.size(); i++)
	{
		if (buffer.find(m_HTTP_Acitve[i]) != buffer.npos)
		{
			return false;
		}
	}

	return true;
}


void MSocket::Request_Handle_L(pConnection cptr, bool& isFlood)
{
	if (!isFlood)
	{
		char* msginfo = new char[m_MSG_Hlen];

		MSG_Head* mesghdr = (MSG_Head*)msginfo;

		//��д��Ϣͷ
		mesghdr->conn = cptr;
		mesghdr->serialNum = cptr->serialNum;

		//����Ϣ��������Ϣ�����в�����
		mthreadpool.GetMassage_And_Proc(msginfo);
	}
	else
	{
		delete cptr->Httprq;
		cptr->Httprq = NULL;
	}

	//���һ�����Ľ��գ���ԭΪ��ʼֵ
	memset(cptr->bufptr, 0, _LINE_MAX_LENGTH);
	cptr->isNewRecv = false;
	cptr->recvStat = _PKG_HD_INIT;

	return;
}


char* MSocket::SendFile_Get(const char* url, ssize_t fsize)
{
	int sendfd = open(url, O_RDONLY);
	if (sendfd == -1)
	{
		sendfd = open("html/404.html", O_RDONLY);
		if (sendfd == -1)
		{
			Error_insert_File(LOG_ERR, "Send file get:> file open failed.");
		}
	}
	
	char* sendfile = new char[fsize + 1];
	char auxget;
	ssize_t gets = 0;
	int n = 0;

	while (gets < fsize)
	{
		n = read(sendfd, &auxget, 1);
		if (n == 0)
		{
			//�ļ���ȡ���
			break;
		}
		if (n != 1)
		{
			//�ļ���ȡʧ��
			delete[]sendfile;
			return NULL;
		}

		sendfile[gets++] = auxget;
	}

	sendfile[gets] = '\0';

	return sendfile;
}


ssize_t MSocket::SendProc(int fd, char* buffer, ssize_t buflen)
{
	ssize_t n;

	while (true)
	{
		n = send(fd, buffer, buflen, 0);

		if (n > 0)
		{
			return n;
		}

		if (n == 0)
		{
			//�Զ����ӶϿ�
			return 0;
		}

		if (errno == EAGAIN)
		{
			return -1;
		}

		if (errno == EINTR)
		{
			//����δ֪�쳣
			Error_insert_File(LOG_ERR, "Send Message:> func SendProc() [errno == EINTR].");
		}
		else
		{
			//��������
			return -2;
		}
	}//end while
}


void MSocket::Write_requestHandle(pConnection cptr)
{
	ssize_t sended = SendProc(cptr->fd, cptr->sendptr, cptr->sendsize);

	if (sended > 0 && sended != cptr->sendsize)
	{
		//����û�з�����
		cptr->sendptr = cptr->sendptr + sended;
		cptr->sendsize -= sended;
		return;
	}
	else if (sended == -1)
	{
		//ϵͳ֪ͨ���Է������ݣ�������ȴ��
		//�������쳣
		Error_insert_File(LOG_ERR,
			"Write request:> send buffer is full.");
		return;
	}

	if (sended > 0 && sended == cptr->sendsize)
	{
		//���ݷ�����ϣ�EPOLLOUT������
		if (Epoll_Add_event(cptr->fd, EPOLL_CTL_MOD, EPOLLOUT,
			1, cptr) == -1)
		{
			Error_insert_File(LOG_CRIT,
				"Send message:> epoll mod delete EPOLLOUT failed.");
		}

		//cerr << "Notice:> Detention message send compelete." << endl;
	}

	delete[]cptr->delptr;
	cptr->delptr = NULL;
	--cptr->throwSendCount;
	ActiveShutdown(cptr);

	//���ź�����ֵ��һ,�ÿ���sem_wait���̼߳���ִ��
	if (sem_post(&m_semSendMsg) == -1)
	{
		Error_insert_File(LOG_CRIT,
			"MsgSend func:> sem_post made a mistake.");
	}

	return;
}


void MSocket::MsgSend(char* sendBuf)
{
	//������Ϣ����
	Mlock lock(&m_sendMsgMutex);

	//��ֹ�����������ݰ�����
	if (m_sendMsgCount > m_MAX_Send)
	{
		m_discardPkg++;
		delete[]sendBuf;

		return;
	}

	//��������Ƿ���ڶ��������հ�����Ϊ
	MSG_Head* pmesghdr = (MSG_Head*)sendBuf;
	pConnection aptr = pmesghdr->conn;

	//��������������δ�������ݹ���
	if (aptr->requestSent > m_MAX_rqSend)
	{
		m_discardPkg++;
		delete[]sendBuf;

		ActiveShutdown(aptr);

		return;
	}


	//��Ϣ������Ϣ����
	++aptr->requestSent;
	m_sendQueue.push_back(sendBuf);
	++m_sendMsgCount;

	//���ź�����ֵ��һ,�ÿ���sem_wait���̼߳���ִ��
	if (sem_post(&m_semSendMsg) == -1)
	{
		Error_insert_File(LOG_CRIT,
			"MsgSend func:> sem_post made a mistake.");
	}

	return;
}



void* MSocket::MessageSend_Thread(void* threadData)
{
	//��ȡ��ָ��
	ThreadItem* pthread = static_cast<ThreadItem*>(threadData);
	MSocket* psock = pthread->pThis;

	int err;
	char* pMsgbuf;

	MSG_Head* mesghdr;
	char* purl;
	pConnection cptr;
	ssize_t pkglen;

	list<char*>::iterator pos, posEnd, aupos;

	while (IsExit == 0)
	{
		//ʹ���ź������̵߳ȴ�
		if (sem_wait(&psock->m_semSendMsg) == -1)
		{
			if (errno == EINTR)
			{
				Error_insert_File(LOG_WARN,
					"epoll_wait():> interrupted system call.");
			}
		}

		//���ǳ���Ҫ�˳�
		if (IsExit != 0)
		{
			break;
		}

		if (psock->m_sendMsgCount > 0)
		{
			//����
			err = pthread_mutex_lock(&psock->m_sendMsgMutex);
			if (err != 0)
			{
				Error_insert_File(LOG_FATAL, 
					"Mutex lock set failed, the error code is %d.", err);
			}

			pos = psock->m_sendQueue.begin();
			posEnd = psock->m_sendQueue.end();

			while (pos != posEnd)
			{
				ssize_t sended = 0;
				pMsgbuf = (*pos);

				//�ӷ������ݻ������л�ȡ��Ϣ
				mesghdr = (MSG_Head*)pMsgbuf;
				purl = pMsgbuf + psock->m_MSG_Hlen;
				cptr = mesghdr->conn;

				//�ж��ų����ڰ�
				if (cptr->serialNum != mesghdr->serialNum)
				{
					//������ڣ�ɾ��������ָ�����Ϣ
					aupos = pos;
					pos++;
					psock->m_sendQueue.erase(aupos);
					--psock->m_sendMsgCount;
					delete[]pMsgbuf;
					pMsgbuf = NULL;
					continue;
				}

				if (cptr->throwSendCount > 0)
				{
					//ӵ�иñ�ǵ���Ϣ��ϵͳ�źż������ͻ
					//�˴�ֱ������
					pos++;
					continue;
				}

				//����Ϊ��Ϣ����
				--cptr->requestSent;

				cptr->delptr = pMsgbuf;

				aupos = pos;
				pos++;
				psock->m_sendQueue.erase(aupos);
				--psock->m_sendMsgCount;

				if (psock->ResponseHeader_Send(cptr, purl))
				{
					cptr->sendptr = psock->SendFile_Get(purl, cptr->sendsize);
					cptr->fdelptr = cptr->sendptr;

					sended = psock->SendProc(cptr->fd, cptr->sendptr, cptr->sendsize);

					if (sended > 0)
					{
						if (sended == cptr->sendsize)
						{
							delete[]cptr->delptr;
							cptr->delptr = NULL;

							psock->ActiveShutdown(cptr);
							//cerr << "SendProc:> Message send success." << endl;
						}
						else
						{
							//���·�������
							cptr->sendptr = cptr->sendptr + sended;
							cptr->sendsize -= sended;

							//����ϵͳ֪ͨ������Ϣ
							++cptr->throwSendCount;

							if (psock->Epoll_Add_event(cptr->fd, EPOLL_CTL_MOD, EPOLLOUT,
								0, cptr) == -1)
							{
								Error_insert_File(LOG_CRIT,
									"Send message:> epoll mod add EPOLLOUT failed.");
							}

							Error_insert_File(LOG_INFO,
								"Send message:> send buffer is full, use system call.");
						}//end if

						continue;
					}//end if (sended > 0)

					else if (sended == 0)
					{
						//Ԥ��֮����쳣����
						Error_insert_File(LOG_ERR, "Send message:> sended is equal 0.");

						delete[]cptr->delptr;
						cptr->delptr = NULL;

						psock->ActiveShutdown(cptr);
					}

					else if (sended == -1)
					{
						//��������������ϵͳ֪ͨ������Ϣ
						++cptr->throwSendCount;

						if (psock->Epoll_Add_event(cptr->fd, EPOLL_CTL_MOD, EPOLLOUT,
							0, cptr) == -1)
						{
							Error_insert_File(LOG_CRIT,
								"Send message:> epoll mod EPOLLOUT failed.");
						}

						Error_insert_File(LOG_INFO,
							"Send message:> send buffer is full, use system call.");

						continue;
					}//end else if

					else
					{
						//ͨ��Ϊ�Զ˶Ͽ�,����Ϊ-2
						delete[]cptr->delptr;
						cptr->delptr = NULL;

						psock->ActiveShutdown(cptr);
					}
				}
			}// end while

			err = pthread_mutex_unlock(&psock->m_sendMsgMutex);
			if (err != 0)
			{
				Error_insert_File(LOG_FATAL, 
					"Mutex unlock set failed, the error code is %d.", err);
			}
		}//end if (psock->m_sendMsgCount > 0)
	}//end while (IsExit != 0)

	return (void*)0;
}


//���಻����ʵ�֣���������̳�
void MSocket::ThreadReceive_Proc(char* msgInfo)
{
	return;
}
