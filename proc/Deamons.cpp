#include "Process.h"
#include "Error.h"

int Create_daemon()//�ػ����̴�������
{
	int fd;

	switch (fork())//�����ӽ��̣��ӽ��̷���0�����ɹ��������̷����ӽ���PID
	{
	case -1:
		Error_insert_File(LOG_FATAL, 
			"Daemons create:> child process create failed.");
		//�ӽ��̴���ʧ�ܼ�¼��־
		return -1;

	case 0:
		//�ӽ��̴������,ֱ������
		break;

	default:
		//�������˳�
		return 1;
	}

	/*----------ֻ���ӽ�����ִ��--------*/

	this_ppid = this_pid;
	this_pid = getpid();
	//��¼�ӽ��̵�pid ppid

	if (setsid() == -1)//Ϊ�ӽ��̽����»Ự��ʹ�������ն�
	{
		Error_insert_File(LOG_FATAL,
			"Daemons create:> Terminal disconnection failed, new session establishment error.");
		//�»Ự����ʧ�ܣ���¼������־
		return -1;
	}

	umask(0);//�����ļ�Ȩ�ޣ������������

	fd = open("/dev/null", O_RDWR);
	//�򿪿��豸��׼���ض����ӽ����������
	
	if (fd == -1)
	{
		Error_insert_File(LOG_URGENT, 
			"Daemons create:> file:/dev/null open failed.");
		//�򿪿��豸ʧ�ܣ���¼������־
		return -1;
	}

	//�����̵ı�׼���붨�򵽿��豸
	if (dup2(fd, STDIN_FILENO) == -1)//�˴���dup2��Ϊ�����Ƚ�STDIN_FILENO�ر�
	{											//��ֹ��������
		Error_insert_File(LOG_URGENT, 
			"Daemons create:> Standard input orientation failed.");
		//�ض�������ʧ�ܣ���¼������־
		return -1;
	}

	//�����̵ı�׼������򵽿��豸
	if (dup2(fd, STDOUT_FILENO) == -1)
	{
		Error_insert_File(LOG_URGENT, 
			"Daemons create:> Standard output orientation failed.");
		//�ض������ʧ�ܣ���¼������־
		return -1;
	}

	if (fd > STDERR_FILENO)//���fd��ռ�ļ�������>ϵͳ��������׼�ļ�������
	{						//˵������������ȷ

		if (close(fd) == -1)//�ͷ�fd��ռ�ļ��������Ա���Դ����
		{ 
			Error_insert_File(LOG_ERR, 
				"Daemons create:> File descriptor release failed.");
			//�ͷ��ļ�������ʧ��,��¼������־
			return -1;
		}
	}

	return 0;//�ػ����̴����ɹ�
}