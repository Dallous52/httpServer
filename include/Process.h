#ifndef Process
#define Process

#include "Globar.h"
#include "ThreadPool.h"

extern MThreadPool mthreadpool;


//�����ӽ������庯��
void Major_Create_WorkProc();

//�������ļ����������ӽ���
static void Create_WorkProc_ForNums();

//�����ӽ���
static int Create_WorkProc(int ordNum);

//�ӽ��̳�ʼ��
static void WorkProcess_Init(int ordNum);

//���Ľ�����
void SetProcName(string name);

//����environ��������λ��
void MoveEnviron();

//�ӽ��̹�������
static void WorkProc_Cycle(int ordNum);

//�����ػ�����
int Create_daemon();

//���������¼��붨ʱ���¼�
void Process_Events_Timer();

#endif // !Process

