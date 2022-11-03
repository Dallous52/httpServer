#include "ThreadPool.h"
#include "Error.h"
#include "Manage.h"

//��ʼ��ȫ�ֱ���
pthread_mutex_t MThreadPool::m_pMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t MThreadPool::m_pCond = PTHREAD_COND_INITIALIZER;

bool MThreadPool::m_isExit = false;


MThreadPool::MThreadPool()
{
	m_threadRunning = 0;
	
	m_oldTime = 0;
}


//�̴߳���
bool MThreadPool::Create(int num)
{
	ThreadItem* newthread;

	int err;

	m_CreateNum = num;

	for (int i = 0; i < m_CreateNum; i++)
	{
		newthread = new	ThreadItem(this);
		m_threadpool.push_back(newthread);
		
		//����ϵͳ���������߳�		ThreadFuncΪ�߳���ں���
		err = pthread_create(&newthread->Handle, NULL, ThreadFunc, newthread);
		if (err != 0)
		{
			Error_insert_File(LOG_FATAL, 
				"Thread create failed, the error code is %d.", err);

			return false;
		}
	}//end for

	//ȡ������
	vector<ThreadItem*>& auxtp = m_threadpool;
	
	for (int i = 0; i < auxtp.size(); i++)
	{
		if (!auxtp[i]->isRunning)
		{
			usleep(100 * 1000);
			//��Ϣ 100 * 1000 ΢��
			i--;
		}
	}

	return true;
}


void MThreadPool::GetMassage_And_Proc(char* buffer)
{
	//���û�����
	int err = pthread_mutex_lock(&m_pMutex);
	if (err != 0)
	{
		Error_insert_File(LOG_FATAL, 
			"Mutex lock set failed, the error code is %d.", err);
	}

	m_MsgQueue.push_back(buffer);

	++m_MsgQueueCount;

	//ȡ��������
	err = pthread_mutex_unlock(&m_pMutex);
	if (err != 0)
	{
		Error_insert_File(LOG_FATAL, 
			"Mutex unlock set failed, the error code is %d.", err);
	}

	Call();

	return;
}


void MThreadPool::Call()
{
	int err = pthread_cond_signal(&m_pCond);
	//�����������ڻ���һ���ȴ�m_pCond�Ľ���
	if (err != 0)
	{
		Error_insert_File(LOG_FATAL,
			"thread call:> thread weak up failed, the error code is %d.", err);
	}

	if (m_CreateNum == m_threadRunning)
	{
		//�������е��߳��� = �߳�����
		//�߳���������
		time_t auxtime = time(NULL);
		if (auxtime - m_oldTime > 10)
		{
			m_oldTime = auxtime;

			Error_insert_File(LOG_CRIT,
				"thread call:> Insufficient threads, please expand the thread pool.");
		}
	}

	return;
}


void* MThreadPool::ThreadFunc(void* threadData)
{
	ThreadItem* auxThread = static_cast<ThreadItem*>(threadData);
	MThreadPool* cpthreadpool = auxThread->pThis;
	//����ȡ��ָ���̳߳����ָ��(��Ϊ�Ǿ�̬��Ա�����޷�ʹ�� thisָ��)

	int err;

	pthread_t tid = pthread_self();
	//��ȡ����pid

	while (true)
	{
		err = pthread_mutex_lock(&m_pMutex);//���û�����
		if (err != 0)
		{
			Error_insert_File(LOG_FATAL,
				"Mutex lock set failed, the error code is %d.", err);
		}

		//���ڿ����߳��ڽ�����Ϣʱ������������ɺ������Ϣ�����Ƿ�Ϊ���Զ��ж��Ƿ�ִ��
		while (cpthreadpool->m_MsgQueue.size() == 0 && m_isExit == false)
		{
			if (auxThread->isRunning == false)
			{
				auxThread->isRunning = true;
				//ֻ������ʱ���������StopAll����
			}

			pthread_cond_wait(&m_pCond, &m_pMutex);
			//���������������߳̽����ڴ˺����еȴ�
			//��������Ϊ��̬��Ա��ֱ������
		}

		//���������������ʱ��Ҫ�˳�
		//m_isExit������Ϊtrue
		if (m_isExit)
		{
			pthread_mutex_unlock(&m_pMutex);
			break;
		}

		//�߳�������������׼������
		
		char* workbuf = cpthreadpool->m_MsgQueue.front();
		cpthreadpool->m_MsgQueue.pop_front();
		--cpthreadpool->m_MsgQueueCount;

		err = pthread_mutex_unlock(&m_pMutex);
		if (err != 0)
		{
			Error_insert_File(LOG_FATAL, 
				"Mutex unlock set failed, the error code is %d.", err);
		}

		
		//���е���˵���߳̿��Խ������ݴ���
		//�߳�������ԭ����+1
		++cpthreadpool->m_threadRunning;

		//����Ϣ���д���
		msocket.ThreadReceive_Proc(workbuf);

		delete[]workbuf;
		--cpthreadpool->m_threadRunning;

	}//end while
	
	return (void*)0;
}


void MThreadPool::StopAll()
{
	//�������ѽ���ֱ���˳�
	if (m_isExit)
	{
		return;
	}

	m_isExit = true;

			
	//������pthread_cond_wait(&m_pCond, &m_pMutex);���ŵ������߳�
	int err = pthread_cond_broadcast(&m_pCond);
	if (err != 0)
	{
		Error_insert_File(LOG_FATAL, 
			"thread pool exit:> broadcast failed, the error code is %d.", err);

		return;
	}

	vector<ThreadItem*>::iterator it;
	for (it = m_threadpool.begin(); it != m_threadpool.end(); it++)
	{
		pthread_join((*it)->Handle, NULL);
		//�ȴ�һ��������ֹ
	}

	//�ͷŻ������ȱ���
	pthread_mutex_destroy(&m_pMutex);
	pthread_cond_destroy(&m_pCond);
	
	//�ͷ��߳���ռ�ռ�
	for (it = m_threadpool.begin(); it != m_threadpool.end(); it++)
	{
		if (*it)
		{
			delete* it;
		}
	}
	m_threadpool.clear();

	Error_insert_File(LOG_NOTICE, "All threads exit normally.");

	return;
}



void MThreadPool::ClearMsgQueue()
{
	char* auxPoint;

	while (!m_MsgQueue.empty())
	{
		auxPoint = m_MsgQueue.front();
		m_MsgQueue.pop_front();

		delete[]auxPoint;
	}
}


MThreadPool::~MThreadPool()
{
	//������Ϣ��������
	ClearMsgQueue();
}