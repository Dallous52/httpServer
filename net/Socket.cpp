#include "Socket.h"
#include "ProfileCtl.h"
#include "Error.h"
#include "MLock.h"

MSocket::MSocket()
{
	m_listen_port_count = 1;
	m_work_connections = 1;

	//epoll��س�Ա������ʼ��
	m_epollHandle = -1;

	m_MSG_Hlen = sizeof(MSG_Head);
	m_HTTP_Acitve = { "GET","POST","HEAD" };

	//��ֵ���ͳ�ʼ��
	m_sendMsgCount = 0;
	m_connection_all = 0;
	m_connection_free = 0;
	m_connection_reco = 0;
	m_connection_work = 0;
	m_MAX_Send = 0;

	return;
}

bool MSocket::Initialize()
{
	m_listen_port_count = stoi(findProVar("listen_port_count"));

	m_work_connections = stoi(findProVar("work_connections"));

	m_reWaitTime = stoi(findProVar("recovery_wait_time"));

	m_floodCheckStart = stoi(findProVar("flood_check_start"));

	m_floodInterval = stoi(findProVar("flood_interval"));

	m_floodPackages = stoi(findProVar("flood_packages"));

	m_MAX_Send = stoi(findProVar("max_send"));

	m_MAX_rqSend = stoi(findProVar("max_request_send"));

	return Establish_listenSocket();
}


bool MSocket::WrokProcRelevent_Init()
{
	//����������ػ�������ʼ��
	if (pthread_mutex_init(&m_recoveryMutex, NULL) != 0)
	{
		Error_insert_File(LOG_CRIT,
			"MSocket initialization:> recycle queue mutex init failed.");
		return false;
	}
	if (pthread_mutex_init(&m_connectMutex, NULL) != 0)
	{
		Error_insert_File(LOG_CRIT,
			"MSocket initialization:> connection mutex init failed.");
		return false;
	}
	if (pthread_mutex_init(&m_sendMsgMutex, NULL) != 0)
	{
		Error_insert_File(LOG_CRIT,
			"MSocket initialization:> message send mutex init failed.");
		return false;
	}

	//�ź�����ʼ��
	if (sem_init(&m_semSendMsg, 0, 0) == -1)
	{
		Error_insert_File(LOG_URGENT, 
			"MSocket initialization:> The semaphore init failed.");
		return false;
	}

	//�����߳�
	int err;

	//���ݷ����߳�
	ThreadItem* sendConn;
	sendConn = new ThreadItem(this);
	m_auxthread.push_back(sendConn);

	err = pthread_create(&sendConn->Handle, NULL,
		MessageSend_Thread, sendConn);
	if (err != 0)
	{
		Error_insert_File(LOG_URGENT, 
			"MSocket initialization:> Message send thread create failed.");
		return false;
	}


	//���ӻ����߳�
	ThreadItem* recyConn;
	recyConn = new ThreadItem(this);
	m_auxthread.push_back(recyConn);

	err = pthread_create(&recyConn->Handle, NULL, 
		ReConnection_Thread, recyConn);
	if (err != 0)
	{
		Error_insert_File(LOG_URGENT, 
			"MSocket initialization:> Recovery thread create failed.");
		return false;
	}

	return true;
}


bool MSocket::Establish_listenSocket()
{
	int auxSocket;
	int auxPort;				

	struct sockaddr_in serv_addr;//��������ַ�ṹ��
	memset(&serv_addr, 0, sizeof(serv_addr));

	//���ñ��ط�����Ҫ�����ĵ�ַ�Ͷ˿�
	serv_addr.sin_family = AF_INET;							//ѡ��Э����ΪIPV4
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//��������IP��ַ

	for (int i = 0; i < m_listen_port_count; i++)
	{
		//��һ��������ʹ��IPV4Э��
		//�ڶ���������ʹ��TCP����
		//��������������֪��������
		auxSocket = socket(AF_INET, SOCK_STREAM, 0);
		//����socket�׽���
		if (auxSocket == -1)
		{
			//�׽��ֽ���ʧ�ܼ�¼��־��
			Error_insert_File(LOG_FATAL, "The %d socket of process %d establish failed:> %m.",
				i, this_pid, errno);

			return false;
		}
		
		//setsockopt():�����׽��ֲ���
		//�ڶ��������͵�������������ʹ��,һһ��Ӧ
		//�˴�SO_REUSEADDR����ͬһ���˿����׽��ֵĸ���
		//���ڽ��bind������TCP״̬TIME_WAIT�����������
		int reuseaddr = 1;
		if (setsockopt(auxSocket, SOL_SOCKET, SO_REUSEADDR,
			(const void*)&reuseaddr, sizeof(int)) == -1)
		{
			//�׽��ָ�������ʧ�ܣ���¼��־
			Error_insert_File(LOG_URGENT, 
				"The %d socket of process %d reuse address setting failed:> %m.",
				i, this_pid, errno);

			
		}

		int reuseport = 1;
		if (setsockopt(auxSocket, SOL_SOCKET, SO_REUSEPORT,
			(const void*)&reuseport, sizeof(int)) == -1)
		{
			//�˿ڸ�������ʧ�ܣ���¼��־
			Error_insert_File(LOG_URGENT, 
				"The %d socket of process %d reuse port setting failed:> %m.",
				i, this_pid, errno);
		}

		if (Set_NoBlock(auxSocket) == false)
		{
			//�׽��ַ���������ʧ�ܣ���¼��־
			Error_insert_File(LOG_URGENT, 
				"The %d socket of process %d no block setting failed:> %m.",
				i, this_pid, errno);

			return false;
		}

		//����Ҫ�����ĵ�ַ��˿�
		string port = "listen_port";
		port += to_string(i);
		auxPort = stoi(findProVar(port));
		serv_addr.sin_port = htons((in_port_t)auxPort);

		//�󶨷�������ַ�ṹ
		if (bind(auxSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		{
			//��������ַ�ṹ��ʧ�ܣ���¼��־
			Error_insert_File(LOG_URGENT, "The %d socket of process %d server address bind failed:> %m.",
				i, this_pid, errno);
			return false;
		}

		//����2��ʾ���������Ի�ѹδ������������������
		if (listen(auxSocket, ListenBackLog) == -1)
		{
			//�����˿�ʧ��
			Error_insert_File(LOG_URGENT, 
				"The %d socket of process %d listen() to port %d failed:> %m.",
				i, this_pid, auxPort, errno);

			return false;
		}

		Listening_t* socketItem = new Listening_t;
		memset(socketItem, 0, sizeof(Listening_t));
		socketItem->fd = auxSocket;
		socketItem->port = auxPort;

		//�����˿����óɹ�����¼��־
		Error_insert_File(LOG_NOTICE, 
			"The %d socket of process %d listening to port %d Succeeded.",
			i, this_pid, auxPort);

		m_listenSocketList.push_back(socketItem);
	}

	return true;
}



int MSocket::epoll_init()
{
	//����epoll����
	m_epollHandle = epoll_create(m_work_connections);
	if (m_epollHandle == -1)
	{
		Error_insert_File(LOG_FATAL, "epoll_init():> Epoll create failed.");
		exit(2);
	}

	InitConnections();

	//m_connection = new vConnection[m_work_connections];
	////Ϊ���ӳط���ռ�

	//pConnection nexts = NULL;
	pConnection head;
	//���ӳ������׵�ַ
	
	vector<Listening_p>::iterator it;
	for (it = m_listenSocketList.begin(); it != m_listenSocketList.end(); it++)
	{
		head = Get_connection((*it)->fd);
		if (head == NULL)
		{
			//��Get_connection�������Ѿ���¼�˴��󣬴˴����ټ�¼
			exit(2);
		}

		//�����Ӷ��������������й���
		head->listening = (*it);
		(*it)->connection = head;

		//�Լ����˿ڵĶ��¼�����������
		head->rHandle = &MSocket::Event_accept;
	
		if (Epoll_Add_event((*it)->fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP, 0,
		 head) == -1)
		{
			exit(2);
		}
	}//end for

	return 1;
}


int MSocket::epoll_process_events(int time)
{
	//�ȴ�����ȡ�¼�
	int event = epoll_wait(m_epollHandle, m_events, MAX_EVENTS, time);
	//event �����¼�����

	//�д������������
	if (event == -1)
	{
		//....
		if (errno == EINTR)
		{
			Error_insert_File(LOG_WARN, "epoll_wait():> interrupted system call.");
			//�����������������1
			return 1;
		}
		else
		{
			Error_insert_File(LOG_ERR, "epoll_wait():> %m", errno);

			//�쳣����
			return 0;
		}
	}

	//�ȴ�ʱ���þ�
	if (event == 0)
	{
		//�ȴ�ʱ�������þ�
		if (time != -1)
		{
			return 1;
		}
		else
		{
			//�쳣״�������޵ȴ�ʱ���þ�
			Error_insert_File(LOG_ERR, "epoll_wait():> infinite waiting time exhausted.");
			return 0;
		}
	}

	//cerr << "Debug:> envents = " << event << " | "
		//<< "pid = " << this_pid << endl;

	//�¼�����������д���
	pConnection cptr;
	//uintptr_t instance;
	uint32_t revents;
	
	for (int i = 0; i < event; i++)
	{
		//����ǰ�������ӳص����Ӻ�instance��ȡ����
		cptr = (pConnection)(m_events[i].data.ptr);

		//instance = (uintptr_t)cptr & 1;				
		////�����㣬��xxxx...xx(1/0) & 0000...001��
		//cptr = (pConnection)((uintptr_t)cptr & (uintptr_t)~1);
		////��xxxxx...xx[1/0] & 1111...110��

		if (cptr->fd == -1)
		{
			//�����Ѿ�����Ĺ����¼���ֱ������
			Error_insert_File(LOG_WARN, "epoll_wait:> skip expiration event.");
			continue;
		}

		//if (cptr->instance != instance)
		//{
		//	Error_insert_File(LOG_WARN, 
		//		"epoll_wait:> skip expiration event(var: instance is changed).");
		//	continue;
		//}

		revents = m_events[i].events;
		
		//���յ��û��Ĺر����������һЩ����ʱ
		if (revents & (EPOLLERR | EPOLLHUP))
		{
			revents |= EPOLLIN | EPOLLOUT;
			//���ùر������Ĵλ��ֵ� �� д ׼��
		}

		//���յ����¼�
		if (revents & EPOLLIN)
		{
			//���ö��¼�������Event_accept
			(this->*(cptr->rHandle))(cptr);
		}

		if (revents & EPOLLOUT)
		{
			if (revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
			{
				//�ͻ��˹ر�
				--cptr->throwSendCount;
			}
			else
			{
				(this->*(cptr->wHandle))(cptr);
			}
		}
	}//end for
	
	return 1;
}


int MSocket::Epoll_Add_event(int sock, uint32_t eventTp,
	uint32_t flag, int addAction, pConnection cptr)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));

	if (eventTp == EPOLL_CTL_ADD)
	{
		//epoll�����ӽڵ�
		//ev.data.ptr = (void*)(cptr);
		ev.events = flag;
		cptr->events = flag;
	}
	else if (eventTp == EPOLL_CTL_MOD)
	{
		//�޸Ľڵ���Ϣ
		ev.events = cptr->events;
		
		if (addAction == 0)
		{
			//���ӱ��
			ev.events |= flag;
		}
		else if (addAction == 1)
		{
			//ɾ�����
			ev.events &= ~flag;
		}
		else
		{
			//��ȫ���Ǳ��
			ev.events = flag;
		}

		cptr->events = ev.events;//���¼�¼���
	}
	else
	{
		//ɾ���ڵ�
		//....
		return 1;
	}

	ev.data.ptr = (void*)(cptr);

	if (epoll_ctl(m_epollHandle, eventTp, sock, &ev) == -1)
	{
		Error_insert_File(LOG_FATAL, "Epoll event addition failed.");
		return -1;
	}

	return 1;
}


//���ý��շ�ʽ������
bool MSocket::Set_NoBlock(int sock)
{
	int nb = 1;
	
	//FIONBIO:���÷�������ǣ�0�����1����(nb)
	if (ioctl(sock, FIONBIO, &nb) == -1)
	{
		return false;
	}

	return true;
}


void MSocket::ActiveShutdown(pConnection cptr)
{
	if (cptr->fd != -1)
	{
		close(cptr->fd);
		cptr->fd = -1;
	}

	if (cptr->throwSendCount > 0)
	{
		cptr->throwSendCount = 0;
	}

	IntoRecovery(cptr);

	return;
}


//�رռ����˿�
void MSocket::Close_listenSocket()
{
	for (int i = 0; i < m_listen_port_count; i++)
	{
		close(m_listenSocketList[i]->fd);

		//�رռ�¼��־
		Error_insert_File(LOG_URGENT, 
			"The number %d listening port has been closed.", i);
	}
}


void MSocket::ClearSendQueue()
{
	list<char*>::iterator it;

	for (it = m_sendQueue.begin(); it != m_sendQueue.end(); it++)
	{
		delete[](*it);
		--m_sendMsgCount;
	}

	m_sendQueue.clear();

	return;
}


void MSocket::WrokProcRelevent_Off()
{
	IsExit = true;

	vector<ThreadItem*>::iterator it;
	//�ȴ������߳���ֹ
	for (it = m_auxthread.begin(); it != m_auxthread.end(); it++)
	{
		pthread_join((*it)->Handle, NULL);
	}

	//�ͷ��߳��ڴ�
	for (it = m_auxthread.begin(); it != m_auxthread.end(); it++)
	{
		if (*it)
		{
			delete (*it);
		}
	}
	m_auxthread.clear();

	//��շ��Ͷ���
	ClearSendQueue();

	//�ͷ����ӳ�
	ClearConnections();

	//�ͷŻ�����
	pthread_mutex_destroy(&m_connectMutex);
	pthread_mutex_destroy(&m_recoveryMutex);
	pthread_mutex_destroy(&m_sendMsgMutex);

	//�ͷ��ź���
	sem_destroy(&m_semSendMsg);

	return;
}


//���������ͷ��ڴ�
MSocket::~MSocket()
{
	for (vector<Listening_t*>::iterator it = m_listenSocketList.begin();
		it != m_listenSocketList.end(); it++)
	{
		delete (*it);
	}

	m_listenSocketList.clear();
	
	return;
}