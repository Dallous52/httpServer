#include "Socket.h"
#include "Error.h"
#include "MLock.h"


Connection::Connection()
{
	serialNum = 0;
	pthread_mutex_init(&logicMutex, NULL);
}


Connection::~Connection()
{
	pthread_mutex_destroy(&logicMutex);
}


void Connection::GetOne()
{
	++serialNum;

	recvStat = _PKG_HD_INIT;
	//Ϊ��ȡ�Ŀ������Ӹ�ֵ(��ʼ��)
	Httprq = new HttpRequest;
	bufptr = new char[_LINE_MAX_LENGTH];
	memset(bufptr, 0, _LINE_MAX_LENGTH);

	throwSendCount = 0;
	events = 0;
	isNewRecv = false;
	sendsize = 0;

	floodCount = 0;
	floodCheckLast = 0;
	requestSent = 0;

	delptr = NULL;
	sendptr = NULL;
	fdelptr = NULL;
}


void Connection::FreeOne()
{
	++serialNum;
	
	//�ͷ�ǰ���ֶ���������ڴ�
	if (Httprq != NULL)
	{
		delete Httprq;
		Httprq = NULL;
	}

	if (bufptr != NULL)
	{
		delete[]bufptr;
		bufptr = NULL;
	}

	if (delptr != NULL)
	{
		delete[]delptr;
		delptr = NULL;
	}
	
	if (fdelptr != NULL)
	{
		delete[]fdelptr;
		fdelptr = NULL;
	}

	throwSendCount = 0;
}


void MSocket::InitConnections()
{
	pConnection pConn;

	for (int i = 0; i < m_work_connections; i++)
	{
		pConn = new Connection;  

		memset(pConn, 0, sizeof(Connection));

		pConn->GetOne();

		m_connection.push_back(pConn);
		m_fconnection.push_back(pConn);
	}

	m_connection_all = m_connection_free = m_connection.size();
	
	return;
}


pConnection MSocket::Get_connection(int sock)
{
	Mlock lock(&m_connectMutex);

	pConnection auxptr;

	//�������������������
	if (!m_fconnection.empty())
	{
		auxptr = m_fconnection.front();
		m_fconnection.pop_front();
		--m_connection_free;

		auxptr->GetOne();
		auxptr->fd = sock;

		return auxptr;
	}

	//�������������û������
	//����һ������

	auxptr = new Connection;

	memset(auxptr, 0, sizeof(Connection));
	auxptr->GetOne();
	m_connection.push_back(auxptr);
	++m_connection_all;

	auxptr->fd = sock;

	return auxptr;
}


void MSocket::Close_Connection(pConnection cptr)
{
	if (close(cptr->fd) == -1)
	{
		Error_insert_File(LOG_URGENT, "Close accept connection:> socket close failed.");
	}

	cptr->fd = -1;

	Free_connection(cptr);

	return;
}


void MSocket::IntoRecovery(pConnection cptr)
{
	list<pConnection>::iterator pos;
	bool isExist = false;

	//cerr << m_reConnection.size() << endl;

	Mlock lock(&m_recoveryMutex);

	for (pos = m_reConnection.begin(); pos != m_reConnection.end(); pos++)
	{
		if (*pos == cptr)
		{
			isExist = true;
			break;
		}
	}
	
	//��֤���Ӳ��ظ�����
	if (isExist)
	{
		return;
	}

	cptr->recyTime = time(NULL);
	++cptr->serialNum;

	m_reConnection.push_back(cptr);
	++m_connection_reco;
	--m_connection_work;

	return;
}


void* MSocket::ReConnection_Thread(void* threadData)
{
	//cerr << "thread recovery function is running." << endl;

	ThreadItem* pThread = static_cast<ThreadItem*>(threadData);
	MSocket* psock = pThread->pThis;
	//����ȡ��ָ���̳߳����ָ��(��Ϊ�Ǿ�̬��Ա�����޷�ʹ�� thisָ��)

	time_t times;

	int err;
	
	list<pConnection>::iterator pos;
	pConnection cptr;

	while (true)
	{
		usleep(200 * 1000);
		//��Ϣ200ms
		//cerr << psock->m_connection_reco << endl;
		if (psock->m_connection_reco > 0)
		{
			times = time(NULL);

			if (psock->m_reConnection.begin() != psock->m_reConnection.end())
			{
				pos = psock->m_reConnection.begin();
				cptr = (*pos);

				//�ж��Ƿ����˳�ʱ��
				//cerr << cptr->recyTime + psock->m_reWaitTime - times << endl;
				if ((cptr->recyTime + psock->m_reWaitTime) > times && !IsExit)
				{
					sleep(2);
					continue;
				}

				//....
				
				//����
				err = pthread_mutex_lock(&psock->m_recoveryMutex);
				if (err != 0)
				{
					Error_insert_File(LOG_FATAL, 
						"Mutex lock set failed, the error code is %d.", err);
				}

				psock->m_reConnection.erase(pos);

				psock->Free_connection(cptr);
				--psock->m_connection_reco;
				//������������ӹ黹

				//cerr << "connection was return 1." << endl;

				err = pthread_mutex_unlock(&psock->m_recoveryMutex);
				if (err != 0)
				{
					Error_insert_File(LOG_FATAL, 
						"Mutex unlock set failed, the error code is %d.", err);
				}
			}//end if (psock->m_reConnection.begin() != psock->m_reConnection.end())

		}//end if (psock->m_connection_reco > 0)
		
		if (IsExit == 1)
		{
			if (psock->m_connection_reco > 0)
			{
				err = pthread_mutex_lock(&psock->m_recoveryMutex);
				if (err != 0)
				{
					Error_insert_File(LOG_FATAL, 
						"Mutex lock set failed, the error code is %d.", err);
				}

				while (psock->m_reConnection.begin() != psock->m_reConnection.end())
				{
					pos = psock->m_reConnection.begin();
					cptr = (*pos);

					//....

					psock->m_reConnection.erase(pos);

					psock->Free_connection(cptr);
					--psock->m_connection_reco;
					//������������ӹ黹
				}

				//cerr << "connection was return 2." << endl;

				err = pthread_mutex_unlock(&psock->m_recoveryMutex);
				if (err != 0)
				{
					Error_insert_File(LOG_FATAL, 
						"Mutex unlock set failed, the error code is %d.", err);
				}
			}//end if
			break;
		}//end if (IsExit == 1)
	}//end while

	return (void*)0;
}


bool MSocket::TestFlood(pConnection cptr)
{
	struct timeval sCurrent;
	uint64_t msCurrent;
	bool isFlood = false;

	//��ȡ��ǰʱ��ṹ
	gettimeofday(&sCurrent, NULL);
	//ʱ��ṹת��Ϊms
	msCurrent = sCurrent.tv_sec * 1000 + sCurrent.tv_usec / 1000;

	if (msCurrent - cptr->floodCheckLast < m_floodInterval)
	{
		cptr->floodCount++;
		cptr->floodCheckLast = msCurrent;
	}
	else
	{
		cptr->floodCount = 0;
		cptr->floodCheckLast = msCurrent;
	}

	if (cptr->floodCount > m_floodPackages)
	{
		isFlood = true;
	}

	return isFlood;
}


void MSocket::Free_connection(pConnection cptr)
{
	Mlock lock(&m_connectMutex);

	cptr->FreeOne();

	m_fconnection.push_back(cptr);

	//���нڵ���+1
	++m_connection_free;

	return;
}


void MSocket::ClearConnections()
{
	pConnection paux;

	while (!m_connection.empty())
	{
		paux = m_connection.front();
		m_connection.pop_front();

		paux->~Connection();

		delete paux;
	}

	return;
}