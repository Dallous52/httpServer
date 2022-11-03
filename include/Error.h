#ifndef Error
#define Error

#include "Globar.h"

using namespace std;

#define LOG_FATAL		0 //����
#define LOG_URGENT		1 //����
#define LOG_SERIOUS		2 //����
#define LOG_CRIT		3 //����
#define LOG_ERR			4 //����
#define LOG_WARN		5 //���� 
#define LOG_NOTICE		6 //ע��
#define LOG_INFO		7 //��Ϣ
#define LOG_DEBUG		8 //����

#define ERR_NUM			9

//������������
const string ErrorInfo[ERR_NUM] = {
	"fatal",
	"urgent",
	"serious",
	"critical",
	"error",
	"warn",
	"notice",
	"infomation",
	"debuge"
};

const string WeekDay[7] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

const string Month[12] = {
	"Jan","Feb","Mar","Apr","May","Jun",
	"Jul","Aug","Sep","Oct","Nov","Dec"
};

//����־������������    ����ȼ�            ��������
void Error_insert_File(int errLev, const char* error, ...);

//��ȡ��ǰʱ��
string GetTime();

//��ȡ��Ӧ����ʱ��
string GetResponseTime();

// ͨ��stat�ṹ�� ����ļ���С����λ�ֽ�
size_t getFileSize(const char* fileName);

#endif