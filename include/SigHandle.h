#ifndef SIGHANDLE
#define SIGHANDLE

#include "Globar.h"

//�洢�ź���Ϣ�ṹ
typedef struct Signal_t
{
	//�źŶ�Ӧ����
	int signal;

	//�ź���
	const char* signame;

	//�źŴ�����ָ��
	void (*sighandle)(int sig, siginfo_t* siginfo, void* ucontext);
}Signal_t;

//�źŴ����ʼ��
int Init_signals();

//�źŴ�����
void signal_handle(int sig, siginfo_t* siginfo, void* ucontext);

//��ȡ�ӽ���״̬��ֹ������ʬ����
void Process_Get_Status();

#endif // !SIGHANDLE

