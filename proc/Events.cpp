#include "Process.h"
#include "Manage.h"

//���������¼��붨ʱ���¼�
void Process_Events_Timer()
{
	msocket.epoll_process_events(-1);

	//........
}