#ifndef MLock
#define MLock

#include "Globar.h"

//�����Զ��ӽ����ļ���
class Mlock
{
public:

	Mlock(pthread_mutex_t* pMutex)
	{
		m_Mutex = pMutex;
		pthread_mutex_lock(m_Mutex);
	}

	~Mlock()
	{
		pthread_mutex_unlock(m_Mutex);
	}

private:
	pthread_mutex_t* m_Mutex;

};

#endif // !MLock

