#include "Process.h"
#include "Error.h"
#include "Manage.h"
#include "ProfileCtl.h"

pid_t this_pid = getpid();
pid_t this_ppid = getppid();

MThreadPool mthreadpool;

void Major_Create_WorkProc()
{
	//�����źż�
	sigset_t sig;
	
	//�ÿ��źż�
	sigemptyset(&sig);

	//���źż�������ź�
	sigaddset(&sig, SIGQUIT);//�ն��˳���
	sigaddset(&sig, SIGCHLD);//�ӽ���״̬�ı�
	sigaddset(&sig, SIGALRM);//��ʱ����ʱ
	sigaddset(&sig, SIGIO);//�첽io
	sigaddset(&sig, SIGINT);//�ն��жϷ�
	sigaddset(&sig, SIGHUP);//���ӶϿ�
	sigaddset(&sig, SIGUSR1);//�û��Զ����ź�1
	sigaddset(&sig, SIGUSR2);//�û��Զ����ź�2
	sigaddset(&sig, SIGTERM);//��ֹ
	sigaddset(&sig, SIGWINCH);//�ն˴��ڴ�С�ı�

	//��������������ڶ����������źż����õ������̵������źż���
	//������ǰ����ԭ���źż������ݱ����ڵ���������(���ﲻ����)
	if (sigprocmask(SIG_BLOCK, &sig, NULL) < 0)
	{
		Error_insert_File(LOG_ERR, "system shielding signal seting failed");
	}
	//���㴴��ʧ��Ҳ���жϳ������

	SetProcName("mast process :");
	//������������

	Create_WorkProc_ForNums();//�����ӽ���

	sigemptyset(&sig);

	while (true)
	{
		//sleep(1);

		sigsuspend(&sig);
		//�������źű�����ʱ�����ź�
		//����ʱ���̹��𣬲�ռ��cpuʱ��

		//Error_insert_File(LOG_DEBUG, "this is mast process");
		
		//cout << "you are successed" << endl;
	}
}


void Create_WorkProc_ForNums()
{
	int num = stoi(findProVar("work_process_num"));
	//�������ļ��л�ȡ����
	
	for (int i = 0; i < num; i++)
	{
		Create_WorkProc(i);
		//�������������ӽ���
	}

	return;
}

int Create_WorkProc(int ordNum)
{
	pid_t pid;

	pid = fork();
	
	char* err = new char[100];
	
	switch (pid)//�����ӽ��̣��ӽ��̷���0�����ɹ��������̷����ӽ���PID
	{	
	case -1:
		Error_insert_File(LOG_SERIOUS, 
			"Ord %d: Child process %d creation failed", ordNum, getpid());
		//�ӽ��̴���ʧ�ܼ�¼��־
		return -1;
		
	case 0:
		//�ӽ��̴������,���ò���
		this_ppid = this_pid;
		this_pid = getpid();

		WorkProc_Cycle(ordNum);
		break;

	default:
		//�����̷���ִ�к���
		break;;
	}

	delete[]err;
	return pid;
}


void WorkProc_Cycle(int ordNum)
{
	WorkProcess_Init(ordNum);

	//�ӽ��̱�־
	this_process = PROCESS_CHILD;

	//�����ӽ�����

	string childname = "work process (" + to_string(ordNum) + ')';
	SetProcName(childname);

	//��¼������־
	string note = childname + " is start and running, pid is " + to_string(this_pid) + '.';
	Error_insert_File(LOG_NOTICE, note.c_str());
	note.clear();

	while (true)
	{
		Process_Events_Timer();

		//cerr << "child process" << ordNum << endl;
		//Error_insert_File(LOG_DEBUG, "this is child process");
	}

	msocket.WrokProcRelevent_Off();
	mthreadpool.StopAll();

	note = childname + " is shutdown, pid is " + to_string(this_pid) + '.';
	Error_insert_File(LOG_NOTICE, note.c_str());

	return;
}


void WorkProcess_Init(int ordNum)
{
	sigset_t sig;

	sigemptyset(&sig);

	if (sigprocmask(SIG_BLOCK, &sig, NULL) < 0)//�ſ����������ź�
	{
		Error_insert_File(LOG_ERR, 
			"work process %d: system shielding signal seting failed.", ordNum);
	}

	int threadNum = stoi(findProVar("thread_num"));
	
	//�����˿ڳ�ʼ��
	if (msocket.Initialize() == false)
	{
		Error_insert_File(LOG_URGENT,
			"Listening port initialization failed.");
		exit(-2);
	}
	else
	{
		cout << "Listening port initialization success." << endl;;
	}

	//�����̳߳�
	if (!mthreadpool.Create(threadNum))
	{
		exit(-2);
	}

	//���������������ʼ��
	if (!msocket.WrokProcRelevent_Init())
	{
		Error_insert_File(LOG_FATAL, 
			"Work process init :> work process relevent init failed.");
		exit(-2);
	}

	msocket.epoll_init();

	return;
}