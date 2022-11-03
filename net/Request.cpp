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
		//http首行接收完整，进行处理
		Request_Handle_A(cptr, isFlood);
	}
	else if (cptr->recvStat == _PKG_BD_INIT)
	{
		string tmpstr = cptr->bufptr;
		//http字段及内容接收
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
		//recv系统函数用于接收socket数据

		if (n == 0)
		{
			//客户端正常关闭，回收连接
			Error_insert_File(LOG_NOTICE, "The client shuts down normally.");

			ActiveShutdown(cptr);

			return -1;
		}

		//错误处理
		if (n < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				//在ET模式下可能出现，表示没有收到数据
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
				//对等放重置连接
				//由用户强制关闭导致的正常情况
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

	//恶意包/错误包处理
	if (IsMalicious(cptr->bufptr))
	{
		cptr->recvStat = _PKG_HD_INIT;
		memset(cptr->bufptr, 0, _LINE_MAX_LENGTH);
	}
	else
	{
		//对合法包头进行处理

		cptr->isNewRecv = true;

		string tmpstr = cptr->bufptr;

		//填写http头
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
	
		//开始包体接收
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

		//填写消息头
		mesghdr->conn = cptr;
		mesghdr->serialNum = cptr->serialNum;

		//将信息包放入信息队列中并处理
		mthreadpool.GetMassage_And_Proc(msginfo);
	}
	else
	{
		delete cptr->Httprq;
		cptr->Httprq = NULL;
	}

	//完成一个包的接收，还原为初始值
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
			//文件获取完成
			break;
		}
		if (n != 1)
		{
			//文件读取失败
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
			//对端连接断开
			return 0;
		}

		if (errno == EAGAIN)
		{
			return -1;
		}

		if (errno == EINTR)
		{
			//发生未知异常
			Error_insert_File(LOG_ERR, "Send Message:> func SendProc() [errno == EINTR].");
		}
		else
		{
			//其他错误
			return -2;
		}
	}//end while
}


void MSocket::Write_requestHandle(pConnection cptr)
{
	ssize_t sended = SendProc(cptr->fd, cptr->sendptr, cptr->sendsize);

	if (sended > 0 && sended != cptr->sendsize)
	{
		//数据没有发送完
		cptr->sendptr = cptr->sendptr + sended;
		cptr->sendsize -= sended;
		return;
	}
	else if (sended == -1)
	{
		//系统通知可以发送数据，缓冲区却满
		//意料外异常
		Error_insert_File(LOG_ERR,
			"Write request:> send buffer is full.");
		return;
	}

	if (sended > 0 && sended == cptr->sendsize)
	{
		//数据发送完毕，EPOLLOUT标记清空
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

	//将信号量的值加一,让卡在sem_wait的线程继续执行
	if (sem_post(&m_semSendMsg) == -1)
	{
		Error_insert_File(LOG_CRIT,
			"MsgSend func:> sem_post made a mistake.");
	}

	return;
}


void MSocket::MsgSend(char* sendBuf)
{
	//发送消息互斥
	Mlock lock(&m_sendMsgMutex);

	//防止滞留发送数据包过多
	if (m_sendMsgCount > m_MAX_Send)
	{
		m_discardPkg++;
		delete[]sendBuf;

		return;
	}

	//检查连接是否存在恶意请求不收包的行为
	MSG_Head* pmesghdr = (MSG_Head*)sendBuf;
	pConnection aptr = pmesghdr->conn;

	//单个连接滞留的未发送数据过多
	if (aptr->requestSent > m_MAX_rqSend)
	{
		m_discardPkg++;
		delete[]sendBuf;

		ActiveShutdown(aptr);

		return;
	}


	//消息放入消息队列
	++aptr->requestSent;
	m_sendQueue.push_back(sendBuf);
	++m_sendMsgCount;

	//将信号量的值加一,让卡在sem_wait的线程继续执行
	if (sem_post(&m_semSendMsg) == -1)
	{
		Error_insert_File(LOG_CRIT,
			"MsgSend func:> sem_post made a mistake.");
	}

	return;
}



void* MSocket::MessageSend_Thread(void* threadData)
{
	//获取类指针
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
		//使用信号量让线程等待
		if (sem_wait(&psock->m_semSendMsg) == -1)
		{
			if (errno == EINTR)
			{
				Error_insert_File(LOG_WARN,
					"epoll_wait():> interrupted system call.");
			}
		}

		//若是程序要退出
		if (IsExit != 0)
		{
			break;
		}

		if (psock->m_sendMsgCount > 0)
		{
			//加锁
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

				//从发送数据缓冲区中获取信息
				mesghdr = (MSG_Head*)pMsgbuf;
				purl = pMsgbuf + psock->m_MSG_Hlen;
				cptr = mesghdr->conn;

				//判断排除过期包
				if (cptr->serialNum != mesghdr->serialNum)
				{
					//如果过期，删除迭代器指向的消息
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
					//拥有该标记的信息由系统信号激发发送活动
					//此处直接跳过
					pos++;
					continue;
				}

				//以下为消息发送
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
							//更新发送零量
							cptr->sendptr = cptr->sendptr + sended;
							cptr->sendsize -= sended;

							//依靠系统通知发送信息
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
						//预料之外的异常错误
						Error_insert_File(LOG_ERR, "Send message:> sended is equal 0.");

						delete[]cptr->delptr;
						cptr->delptr = NULL;

						psock->ActiveShutdown(cptr);
					}

					else if (sended == -1)
					{
						//缓冲区满，依靠系统通知发送信息
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
						//通常为对端断开,返回为-2
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


//父类不进行实现，用于子类继承
void MSocket::ThreadReceive_Proc(char* msgInfo)
{
	return;
}
