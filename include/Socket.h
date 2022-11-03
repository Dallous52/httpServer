#ifndef Sockets
#define Sockets

#include "Globar.h"
#include "Command.h"

//���������Ի�ѹδ������������������
#define ListenBackLog 511							  

//�¼��������
#define MAX_EVENTS	  512	

//*****************ͨѶ״̬��ض���*******************

#define _LINE_MAX_LENGTH 2000						  //ÿ����󳤶�

//ͨѶ�հ�״̬����

#define _PKG_HD_INIT    0							  //��ͷ����׼��״̬

#define _PKG_BD_INIT    3							  //�������׼��״̬

#define HEAD_BUFFER		20							  //��ͷ���ջ�����

#pragma pack(1)//����ϵͳĬ�϶����ֽ���Ϊ 1

////��ͷ�ṹ����
//typedef struct PKG_Head
//{
//	unsigned short pkgLen;					//�����ܳ�
//	unsigned short mesgCode;				//�������ֱ�������
//
//	int	crc32;								//crc32����У�顾ѭ������У�顿
//}PKG_Head;

#pragma pack()//��ԭϵͳĬ�϶����ַ���

//****************************************************

typedef struct Connection  vConnection, *pConnection;	//ָ��������Ϣ�ṹָ��

typedef struct Listening   Listening_t, *Listening_p;	//ָ������˿ڽṹָ��

typedef class MSocket MSocket;

//�¼�������ָ��
typedef void (MSocket::* Event_handle)(pConnection head);


//���ӳ������ӽṹ
struct Connection
{
	//******��ʼ�����ͷ�******
	Connection();
	~Connection();
	void GetOne();					//����ȡ�򴴽�һ������ʱִ�д˺�����ʼ����Ա
	void FreeOne();					//��Ҫ�ͷ�һ������ʱ���ô˺���
	//************************


	//******���ӻ�����Ϣ******
	int fd;							//�׽��־��
	uint32_t events;				//�����ӵ��¼���������
	Listening_p listening;			//�����ṹ��ָ��
	uint8_t serialNum;				//������ţ�ÿ�η���ʱ��һ
	struct sockaddr	s_addr;			//���ڱ���Է���ַ��Ϣ
	
	Event_handle rHandle;			//���¼�������غ���
	Event_handle wHandle;			//д�¼�������غ���

	//uint8_t w_ready;				//��unsigned char��д����׼����ɱ��
	//uint8_t r_ready;				//������׼�����
	//unsigned instance : 1;		//Ч�ñ�־λ����С1bit ,0����Ч��1��ʧЧ
	//*************************


	//********�հ��й�*********
	unsigned char recvStat;			//��ǰ�հ�״̬
	bool isNewRecv;					//�жϰ�ͷ�Ƿ���ɽ��գ��Է���ռ���հ���
	
	char*	bufptr;					//��ʼָ���ͷ��������Ԫ�أ����ڸ�������
	HttpRequest* Httprq;			//���ڴ���������Ϣ
	//*************************


	//*********�������*********
	char* delptr;					//���ڷ������ݺ��ͷ��ڴ��ָ��

	char* fdelptr;					//�����ͷŷ����ļ���Ϣ������
	char* sendptr;					//���淢���ļ���Ϣ�Ļ�����
	ssize_t sendsize;				//��Ҫ���͵��ֽ���

	atomic<int> requestSent;		//�ͻ��������͵���Ϣ��
	atomic<int> throwSendCount;		//�Ƿ�ͨ��ϵͳ���ͣ�> 0 ��ʾͨ��ϵͳ֪ͨ����
	//**************************


	//*********��ȫ���*********
	uint64_t floodCheckLast;		//���һ�η�����ʱ��
	size_t floodCount;				//�������ɰ�����
	//**************************


	//**********����***********
	time_t recyTime;				//���ӿ�ʼ����ʱ��
	pthread_mutex_t logicMutex;		//ҵ���߼�������������
	pConnection	next;				//���ָ��
};


//�����˿������Ϣ
struct Listening
{
	int port; //�����˿ں�
	int fd;	  //�׽��־��
	
	pConnection connection; //���ӳ���Ԫ��ָ��
};


//��Ϣͷ
typedef struct MSG_Head
{
	pConnection	conn;			//ָ��ָ��ǰ����
	uint64_t serialNum;			//������ţ����ڱȽ������Ƿ�����
}MSG_Head;


//������Ϣ������
class MSocket
{
public:

	MSocket();
	
	//��virtual������������ʵ�ֶ�Ŀ�꺯���ĸ���
	virtual ~MSocket();

public:
	
	//��ʼ������
	virtual bool Initialize();
	
	//epoll��ʼ��
	int epoll_init();

	//���������������ʼ��
	bool WrokProcRelevent_Init();

	//epoll�¼�������
	int epoll_process_events(int time);

	//�����߳̽����źŴ���
	virtual void ThreadReceive_Proc(char* msgInfo);

	//����������عرջ��պ���
	void  WrokProcRelevent_Off();

	//����epoll�¼�
	int Epoll_Add_event(int sock, uint32_t enventTp,
		uint32_t otherflag, int addAction, pConnection cptr);

	//��һ�������͵����ݷ�����Ϣ���еȴ�����
	void MsgSend(char* sendBuf);

	//�����ر�һ������
	void ActiveShutdown(pConnection cptr);

private:

	//�����߳̽ṹ����
	struct ThreadItem
	{
		pthread_t Handle;		//�߳̾��
		MSocket* pThis;			//�̳߳�ָ��

		bool isRunning;			//����Ƿ�ʼ���У�ֻ�п�ʼ���вſ�ͨ��StopAll�ر�
		
		//���캯��ͨ��Ĭ�ϲ�����ʼ����Ա
		ThreadItem(MSocket* pthis) :pThis(pthis), isRunning(false) {}
		
		~ThreadItem() {}
	};

private:
	//���������˿�
	bool Establish_listenSocket();
	
	//�رռ����˿�
	void Close_listenSocket();
	
	//���ü���������
	bool Set_NoBlock(int sock);
	
	//��ʼ�����ӳ�
	void InitConnections();
	
	//�����ӳ��л�ȡһ���������Ӷ���
	pConnection Get_connection(int sock);
	
	//�û������¼�������
	void Event_accept(pConnection old);
	
	//�ر��û���������
	void Close_Connection(pConnection cptr);
	
	//�������ӳ�������������ͷ�
	void Free_connection(pConnection cptr);
	
	//���ӻ��մ����߳�
	static void* ReConnection_Thread(void* threadData);
	
	//���ݷ����߳�
	static void* MessageSend_Thread(void* threadData);
	
	//����Ҫ�ر���յ����ӷ�����ն���
	void IntoRecovery(pConnection cptr);

	//��ջ������ӳ�
	void ClearConnections();

	//�����ݵĶ�������
	void Read_requestHandle(pConnection cptr);

	//�����ݵķ��ʹ�����
	void Write_requestHandle(pConnection cptr);

	//����http�����ĵ�һ��
	ssize_t ReceiveProc(pConnection cptr);

	//��������ר�ú��������ؽ��յ������ݴ�С
	ssize_t SendProc(int fd, char* buffer, ssize_t buflen);

	//��ȡ���͵��ļ�
	char* SendFile_Get(const char* url, ssize_t fsize);

	//������Ӧͷ��
	bool ResponseHeader_Send(pConnection cptr, string url);

	//����html�ļ���Ӧ
	void HtmlResponse_Get(pConnection cptr, vector<string>& body);

	//���ڰ�ͷ������Ϻ�Ĵ���
	void Request_Handle_A(pConnection cptr, bool& isFlood);
	
	//����������ж�
	bool IsMalicious(const string buffer);

	//����������������Ϻ�Ĵ���
	void Request_Handle_L(pConnection cptr, bool& isFlood);
	
	//��շ��Ͷ���
	void ClearSendQueue();

	//��������Ƿ�����˷��鹥��
	bool TestFlood(pConnection cptr);

private:
	
	int m_listen_port_count;					//�����˿���
	vector<Listening_p> m_listenSocketList;		//�׽��ֶ���

	int m_work_connections;						//epoll�����������
	int m_epollHandle;							//epoll_create���ؾ��	
	struct epoll_event m_events[MAX_EVENTS];	//���ڴ���epoll��Ϊ

	list<pConnection>	m_connection;			//���ӳ�����
	list<pConnection> m_reConnection;			//���ӻ�������
	list<pConnection> m_fconnection;			//���ӳؿ�������
	pthread_mutex_t m_connectMutex;				//������ػ�����
	atomic<int> m_connection_all;				//����ʹ��������
	atomic<int> m_connection_free;				//���ÿ���������
	atomic<int> m_connection_reco;				//����������
	atomic<int> m_connection_work;				//���ڹ���������

	list<char*>	m_sendQueue;					//������Ϣ����
	atomic<int> m_sendMsgCount;					//��Ϣ���Ͷ�����Ϣ��
	sem_t m_semSendMsg;							//���Ͷ����ź���
	pthread_mutex_t m_sendMsgMutex;				//������ػ�����
	size_t m_MAX_Send;							//������Ϣ�������������
	size_t m_MAX_rqSend;						//�������ӷ�����Ϣ�������������
	atomic<int> m_discardPkg;					//���Ͷ��ж�����������

	pthread_mutex_t m_recoveryMutex;			//������ػ�����
	int  m_reWaitTime;							//���յȴ�ʱ��

	bool m_floodCheckStart;						//�Ƿ���������
	uint32_t m_floodInterval;					//������������ʱ��
	size_t m_floodPackages;						//���������涨ʱ���������ͨ��������

	vector<ThreadItem*> m_auxthread;			//���ڴ��渨������

public:

	size_t m_MSG_Hlen;							//��Ϣͷ����
	vector<string> m_HTTP_Acitve;					//������֧�ֵ�HTTP����
};

#endif // !Sockets

