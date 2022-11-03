#ifndef Command
#define Command


enum HTTP_RQ_HEAD
{
	ACTIVE,
	URL,
	VERSION
};


#pragma pack(1)

////�ͻ��˷�����Ϣ�ṹ�嶨��
//typedef struct mMessage
//{
//	char message[100];
//	int test;
//
//}mMessage;

//http������Ϣ�ֶνṹ
typedef struct HttpRequest_Body
{
	string name;		//�ײ��ֶ���
	string value;		//�ײ��ֶ�ֵ
}HttpRequest_Body;

//http������Ϣ�ṹ
typedef struct HttpRequest
{
	//http����ͷ����Ϣ�ṹ
	string headinfo[3];

	vector<HttpRequest_Body> body;
}HttpRequest;

#pragma pack()

#endif // !Command

