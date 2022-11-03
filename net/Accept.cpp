#include "Socket.h"
#include "Error.h"

//�½�����ר�ú���
void MSocket::Event_accept(pConnection old)
{
	struct sockaddr serv_sockaddr;//������socket��ַ
	socklen_t socklen;
	int err;
	int level;
	int a_sock;
	static bool use_accept4 = true; //�Ƿ�ʹ��accept4����
	
	pConnection newptr;

	socklen = sizeof(serv_sockaddr);

	Error_insert_File(LOG_DEBUG, "Debug:> A connect comming...");

	do
	{
		if (use_accept4)
		{
			//ʹ��accept4��������ֱ�ӽ��׽�������Ϊ������
			a_sock = accept4(old->fd, &serv_sockaddr, &socklen, SOCK_NONBLOCK);
		}
		else
		{
			//ʹ��accept������Ҫ�ں����ֶ����÷�����
			a_sock = accept(old->fd, &serv_sockaddr, &socklen);
		}

		//���Ӵ�����
		if (a_sock == -1)
		{
			err = errno;

			if (err == EAGAIN)
			{
				//�����˴����ʾacceptû��׼���ã�ֱ���˳�
				return;
			}

			level = LOG_URGENT;

			//��������������ֹ
			if (err == ECONNABORTED)
			{
				level = LOG_ERR;
			}
			else if  (err == ENFILE || err == EMFILE)
			{
				//����fd�þ�
				level = LOG_CRIT;
			}

			Error_insert_File(level, "event accept failed:> %m", errno);

			if (use_accept4 && ENOSYS)
			{
				//ϵͳ��֧��acceot4������ʹ��accept��������һ��
				use_accept4 = false;
				continue;
			}

			if (err == ECONNABORTED)
			{
				//�ɺ��Դ��� �� do nothing
			}

			if (err == EMFILE || err == ENFILE)
			{
				//........
			}

			return;
		}//end if (a_sock == -1)
		
		//----------------------------accept�ɹ�----------------------
		Error_insert_File(LOG_INFO, "accept success");
		
		//�������������������������ϵͳ��æ���ٽ�������
		if (m_connection_work >= m_work_connections)
		{
			close(a_sock);
			return;
		}

		//Ϊ�û������׽����������ӳ�����
		newptr = Get_connection(a_sock);

		//��Ϊ���ӳؿռ䲻�㣬��������ʧ��
		if (newptr == NULL)
		{
			//�ر�����
			if (close(a_sock) == -1)
			{
				Error_insert_File(LOG_URGENT, "epoll accept:> a_socket close failed.");
			}

			return;
		}

		//���ӳ����������ӳɹ�
		
		//���ͻ��˵�ַ���������Ӷ���
		memcpy(&newptr->s_addr, &serv_sockaddr, socklen);

		//����accept�����ķ�����
		if (!use_accept4)
		{
			if (Set_NoBlock(a_sock) == false)
			{
				//���÷�����ʧ��
				Close_Connection(newptr);
				return;
			}
		}

		//�������Ӷ������������
		newptr->listening = old->listening;
		
		//��д���
		//newptr->w_ready = 1;

		newptr->rHandle = &MSocket::Read_requestHandle;

		newptr->wHandle = &MSocket::Write_requestHandle;

		//����epoll�¼�
		//�����ɽ���4������otherflag ����ΪEPOLLET ��Ե����ģʽ
		if (Epoll_Add_event(a_sock, EPOLL_CTL_ADD, EPOLLIN  | EPOLLRDHUP, 0,
			newptr) == -1)
		{
			Close_Connection(newptr);
			return;
		}

		m_connection_work++;
		//cerr << "Debug:> work connections = " << m_connection_work << endl;
		break;

	} while (true);

	return;
}