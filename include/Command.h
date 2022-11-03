#ifndef Command
#define Command


enum HTTP_RQ_HEAD
{
	ACTIVE,
	URL,
	VERSION
};


#pragma pack(1)

////客户端发送消息结构体定义
//typedef struct mMessage
//{
//	char message[100];
//	int test;
//
//}mMessage;

//http请求信息字段结构
typedef struct HttpRequest_Body
{
	string name;		//首部字段名
	string value;		//首部字段值
}HttpRequest_Body;

//http请求信息结构
typedef struct HttpRequest
{
	//http请求头部信息结构
	string headinfo[3];

	vector<HttpRequest_Body> body;
}HttpRequest;

#pragma pack()

#endif // !Command

