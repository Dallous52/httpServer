#include "Manage.h"
#include "Error.h"
#include "ProfileCtl.h"

//�洢�����ļ�����
vector<Profile_Var> GloVar;

//�����ļ�ע���ַ���
vector<char> note = { '#','[','\n' };

void Provar_init()
{
	ifstream file;

	file.open(File_Profile_Ctl, ios::in);

	if (!file.is_open())
	{
		Error_insert_File(LOG_FATAL, "open profile failed:> %m.", errno);
		exit(0);
	}

	//�����ļ�������ʼ��
	char* test = new char[401];
	while (file.getline(test, 400))
	{
		vector<string> ansed = GetConfig_Name_Num(test, note);

		if (ansed.size())
		{
			Profile_Var aux;
			aux.name = ansed[0];
			aux.var = ansed[1];

			GloVar.push_back(aux);
		}
	}
}


void Show_ProFile_var()
{
	for (int i = 0; i < GloVar.size(); i++)
	{
		cout << GloVar[i].name << " = " << GloVar[i].var << endl;
	}
}