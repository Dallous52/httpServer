#ifndef ThreadPool
#define ThreadPool

#include "Globar.h"

class MThreadPool
{
public:

	MThreadPool();

	~MThreadPool();

public:

	//������num���߳�
	bool Create(int num);

	//ʹ�̳߳������е��߳��˳�
	void StopAll();

	//���̳߳ص��߳�ִ������
	void Call();

	//��ȡ��Ϣ�����е���Ϣ��������������	
	void GetMassage_And_Proc(char* buffer);

private:

	//���̵߳��߳���ں���
	static void* ThreadFunc(void *threadData);

	//�����Ϣ����
	void ClearMsgQueue();

private:

	//�߳̽ṹ����
	struct ThreadItem
	{
		pthread_t Handle;			//�߳̾��
		MThreadPool* pThis;			//�̳߳�ָ��

		bool isRunning;				//����Ƿ�ʼ���У�ֻ�п�ʼ���вſ�ͨ��StopAll�ر�

		//���캯��ͨ��Ĭ�ϲ�����ʼ����Ա
		ThreadItem(MThreadPool* pthis) :pThis(pthis), isRunning(false) {}

		~ThreadItem() {}
	};

private:

	int m_CreateNum;					//Ҫ�������߳�����
	vector<ThreadItem*> m_threadpool;	//�̳߳��ڴ�����

	static pthread_mutex_t m_pMutex;	//������
	static pthread_cond_t m_pCond;		//�߳�ͬ����������
	static bool m_isExit;				//�߳��˳���־��trueΪ�˳�
	atomic<int> m_threadRunning;		//�������߳���
	time_t m_oldTime;					//�ϴη����̲߳������¼���ʱ��

	list<char*> m_MsgQueue;				//��Ϣ����
	int m_MsgQueueCount;				//��Ϣ���д�С

};
#endif // !ThreadPool

