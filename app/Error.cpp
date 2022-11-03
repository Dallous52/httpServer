#include "Error.h"
#include "ProfileCtl.h"

void Error_insert_File(int errLev, const char* error, ...)
{
	//��ȡ�ɱ����
	va_list vapa;
	va_start(vapa, error);
	
	char* buf = nullptr;
	vasprintf(&buf, error, vapa);

	int log = open(errfile.c_str(), O_WRONLY | O_APPEND);

	if (log == -1)//��־��ʧ�ܴ���
	{
		log = open(errfile.c_str(), O_WRONLY | O_CREAT, 0600);

		if (log == -1)
		{
			cerr << "log file open failed : " << strerror(errno) << endl;
			return;
		}
	}

	string currTime = GetTime();

	//������Ϣ������־�ļ�
	if (errLev >= 0 && errLev < ERR_NUM)
	{
		if (errLev < fail_lev)//�ж��Ƿ��������Ļ
		{
			cerr << currTime << " [" << ErrorInfo[errLev] << "] " << buf << endl;
		}

		string err = currTime + " [" + ErrorInfo[errLev] + "] " + buf + "\n";
		//********�ǵü�ʱ��
		write(log, err.c_str(), err.size());
	}
	
	int er = close(log);
	if (er == -1)
	{
		cerr << "log file close failed : " << strerror(errno) << endl;
	}
}


string GetTime()
{
	string date;
	time_t times;
	struct tm* timed;
	char ansTime[50];

	time(&times); //��ȡ��1900������˶����룬����time_t���͵�timep
	timed = localtime(&times);//��localtime������ת��Ϊstruct tm�ṹ��

	sprintf(ansTime, "%d-%02d-%02d %02d:%02d:%02d", 1900 + timed->tm_year, 1 + timed->tm_mon,
		timed->tm_mday, timed->tm_hour, timed->tm_min, timed->tm_sec);

	date = ansTime;

	return date;
}


string GetResponseTime()
{
	string date;
	time_t times;
	struct tm* timed;
	char ansTime[50];

	time(&times); //��ȡ��1900������˶����룬����time_t���͵�timep
	timed = localtime(&times);//��localtime������ת��Ϊstruct tm�ṹ��

	sprintf(ansTime, "%02d:%02d:%02d", timed->tm_hour, timed->tm_min, timed->tm_sec);

	date = WeekDay[timed->tm_wday] + ", " + to_string(timed->tm_mday)
		+ " " + Month[timed->tm_mon] + " " + to_string(1900 + timed->tm_year)
		+ " " + ansTime + " GMT\r\n";

	return date;
}


// ͨ��stat�ṹ�� ����ļ���С����λ�ֽ�
size_t getFileSize(const char* fileName) 
{

	if (fileName == NULL) {
		return 0;
	}

	// ����һ���洢�ļ�(��)��Ϣ�Ľṹ�壬�������ļ���С�ʹ���ʱ�䡢����ʱ�䡢�޸�ʱ���
	struct stat statbuf;

	// �ṩ�ļ����ַ���������ļ����Խṹ��
	stat(fileName, &statbuf);

	// ��ȡ�ļ���С
	size_t filesize = statbuf.st_size;

	return filesize;
}