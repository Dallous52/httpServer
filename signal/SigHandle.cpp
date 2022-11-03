#include "SigHandle.h"
#include "Error.h"

//���ź��Լ���Ӧ��������
Signal_t signals[] = {
	{SIGHUP,	"SIGHUP",			signal_handle},
	{SIGINT,	"SIGINT",			signal_handle},
	{SIGQUIT,	"SIGQUIT",			signal_handle},
	{SIGTERM,	"SIGTERM",			signal_handle},
	{SIGCHLD,	"SIGCHLD",			signal_handle},
	{SIGSYS,	"SIGSYS, SIG_IGN",	signal_handle},
	{SIGIO,		"SIGIO",			signal_handle},
	{SIGPIPE,	"SIGPIPE",			signal_handle},
	{	0,		 NULL,				NULL},
};

//��¼�ӽ����Ƿ��˳�
sig_atomic_t this_reap = 0;

int Init_signals()
{
	Signal_t *sig;

	struct sigaction sa;
	//ϵͳ�������źŴ����йؽṹ��

	for (sig = signals; sig->signal != 0; sig++)
	{
		memset(&sa, 0, sizeof(struct sigaction));

		if (sig->sighandle)
		{
			sa.sa_sigaction = sig->sighandle;
			//���źŶ�Ӧ�Ĵ��������Ӹ�ϵͳ�ṹ��
			
			sa.sa_flags = SA_SIGINFO;
			//��ʾ�źŴ��������Ч
		}
		else
		{
			sa.sa_handler = SIG_IGN;
			//��ʾ�����ͺŴ������
		}

		sigemptyset(&sa.sa_mask);
		//�������źż����

		//������������õ�sa���õ�ϵͳ��ȥ
		if (sigaction(sig->signal, &sa, NULL) == -1)
		{
			return -1;
		}
	}

	return 0;
}


void signal_handle(int sig, siginfo_t* siginfo, void* ucontext)
{
	Signal_t* sigs;

	//��Ϣ��ָ�򵽽��յ�����Ϣ�����ڼ�¼��־
	for (sigs = signals; sigs->signal != 0; sigs++)
	{
		if (sigs->signal == sig)
		{
			break;
		}
	}

	if (this_process == PROCESS_MASTER)
	{
		switch (sig)
		{
		case SIGCHLD:
			this_reap = 1;
			//���յ����ź�ʱ��reap��¼�ӽ���״̬���Ա������ָ̻�
			break;

		default:
			
			break;
		}
	}
	else if (this_process == PROCESS_CHILD)
	{
		//.....
	}
	else
	{
		//.....
	}
	
	//������������ź���Ϣ��Ϊ�գ������ź�����pid��Ϊ��
	if (siginfo && siginfo->si_pid)
	{
		Error_insert_File(LOG_NOTICE, "signal %d %s is received from process %d.",
			sigs->signal, sigs->signame, siginfo->si_pid);
	}
	else
	{
		Error_insert_File(LOG_NOTICE, "signal %d %s is received unknow process.",
			sigs->signal, sigs->signame);
	}

	if (sig == SIGCHLD)
	{
		Process_Get_Status();
	}
	
	//cout << "signal is comming" << endl;
	//cout << this_pid << endl;

	return;
}
