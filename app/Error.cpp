#include "Error.h"
#include "ProfileCtl.h"

void Error_insert_File(int errLev, const char* error, ...)
{
	//获取可变参数
	va_list vapa;
	va_start(vapa, error);
	
	char* buf = nullptr;
	vasprintf(&buf, error, vapa);

	int log = open(errfile.c_str(), O_WRONLY | O_APPEND);

	if (log == -1)//日志打开失败处理
	{
		log = open(errfile.c_str(), O_WRONLY | O_CREAT, 0600);

		if (log == -1)
		{
			cerr << "log file open failed : " << strerror(errno) << endl;
			return;
		}
	}

	string currTime = GetTime();

	//错误信息输入日志文件
	if (errLev >= 0 && errLev < ERR_NUM)
	{
		if (errLev < fail_lev)//判断是否输出与屏幕
		{
			cerr << currTime << " [" << ErrorInfo[errLev] << "] " << buf << endl;
		}

		string err = currTime + " [" + ErrorInfo[errLev] + "] " + buf + "\n";
		//********记得加时间
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

	time(&times); //获取从1900至今过了多少秒，存入time_t类型的timep
	timed = localtime(&times);//用localtime将秒数转化为struct tm结构体

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

	time(&times); //获取从1900至今过了多少秒，存入time_t类型的timep
	timed = localtime(&times);//用localtime将秒数转化为struct tm结构体

	sprintf(ansTime, "%02d:%02d:%02d", timed->tm_hour, timed->tm_min, timed->tm_sec);

	date = WeekDay[timed->tm_wday] + ", " + to_string(timed->tm_mday)
		+ " " + Month[timed->tm_mon] + " " + to_string(1900 + timed->tm_year)
		+ " " + ansTime + " GMT\r\n";

	return date;
}


// 通过stat结构体 获得文件大小，单位字节
size_t getFileSize(const char* fileName) 
{

	if (fileName == NULL) {
		return 0;
	}

	// 这是一个存储文件(夹)信息的结构体，其中有文件大小和创建时间、访问时间、修改时间等
	struct stat statbuf;

	// 提供文件名字符串，获得文件属性结构体
	stat(fileName, &statbuf);

	// 获取文件大小
	size_t filesize = statbuf.st_size;

	return filesize;
}