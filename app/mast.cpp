#include "Manage.h"
#include "Error.h"
#include "ProfileCtl.h"
#include "Process.h"
#include "SigHandle.h"

//指向用户输入指令
char* const* m_argv;

//是否以守护进程方式开启
bool isIn_daemon;

//程序是否退出
bool IsExit = false;

//进程类型
uint8_t this_process = 0;

//网络相关事件处理类
LogicSocket msocket; 

//从配置文件获取输出错误等级
int fail_lev;

//从配置文件中获取日志文件名
string errfile;

//从配置文件中获取http信息文件名
string httpinfo_file;

//储存html文件的文件夹名
string html_content;

int main(int argc, char* const* argv)
{
	m_argv = argv;

	//配置文件读入
	Provar_init();

	fail_lev = stoi(findProVar("out_error_level"));
	//从配置文件获取输出错误等级

	errfile = findProVar("log_addr");
	//从配置文件中获取日志文件名

	httpinfo_file = findProVar("httpinfo_addr");
	//从配置文件中获取http信息文件名

	html_content = findProVar("html_content_addr");
	//从配置文件中获取储存html文件的文件夹名

	//信号处理内容初始话
	if (Init_signals() == -1)
	{
		Error_insert_File(LOG_URGENT,
			"signal processing function initialization failed.");
		return 1;
	}
	else
	{
		Error_insert_File(LOG_INFO,
			"signal set initialization success.");
	}

	//manage.Show_ProFile_var();

	//判断是否以守护进程方式打开
	if (stoi(findProVar("start_on_daemons")) == 1)
	{
		int is_start = Create_daemon();

		if (is_start == -1)
		{
			Error_insert_File(LOG_FATAL,
				"Deamons create failed.");
			return 1;
		}
		if (is_start == 1)
		{
			Error_insert_File(LOG_NOTICE,
				"parent process exited");

			return 0;
		}

		isIn_daemon = true;
	}

	MoveEnviron();

	Major_Create_WorkProc();

	return 0;
}

