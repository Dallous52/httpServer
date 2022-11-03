#include "SigHandle.h"
#include "Error.h"

void Process_Get_Status()
{
	pid_t	pid;
	int		stat;
	int		err;
	int		one = 0;

	while (true)
	{
		pid_t pid = waitpid(-1, &stat, WNOHANG);
		//����1��-1 ��ʾ�ȴ��κ��ӽ���
		//����2���洢״̬��Ϣ
		//����3������ϳ���
		
		//pid = 0��ʾ�ӽ���û������ֱ�ӷ���
		if (pid == 0)
		{
			return;
		}
		if (pid == -1)
		{
			err = errno;

			//���ñ�ĳ���ź��ն�
			if (err == EINTR)
			{
				continue;
			}

			//�����Ѿ�ִ�й��ˣ�û���ӽ���ֱ�ӷ���
			if (err == ECHILD && one)
			{
				return;
			}

			//û���ӽ���
			if (err == ECHILD)
			{
				Error_insert_File(LOG_ERR, "waitpid() failed 1.");
				return;
			}

			Error_insert_File(LOG_ERR, "waitpid() failed 2.");
			return;
		}

		//�˴���ʾ�ӽ����Ѿ��ɹ��˳�����¼�˳���־

		one = 1;

		if (WTERMSIG(stat))//��ȡ�ӽ�����ֹ�źű��
		{
			//��¼�ӽ����˳���־
			Error_insert_File(LOG_WARN, "process %d is exited on signal %d",
				pid, WTERMSIG(stat));
		}
		else
		{
			//��¼�ӽ����˳���־
			Error_insert_File(LOG_NOTICE, "process %d is exited on signal %d",
				pid, WTERMSIG(stat));
		}
	}

	return;
}