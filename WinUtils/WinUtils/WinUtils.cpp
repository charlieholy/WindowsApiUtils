// WinUtils.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <tlhelp32.h>
#include <time.h>
#include <string>
#include <memory>
#include <iostream>
#include <mutex>
#pragma warning( disable : 4996 )

#define cgiStrEq(a, b) (!strcmp((a), (b)))

enum UtSystemCmdEnum
{
	NONE,
	STOP,
	START,
};
std::mutex __m_mtx;
static int g_UtSystemCmd;

bool Ut_isStart(std::string _exe)
{
	bool bRet = false;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot) {
		printf("��ȡ�����б�ʧ��");
		return bRet;
	}
	PROCESSENTRY32 pe = { sizeof(pe) };
	for (BOOL ret = Process32First(hSnapshot, &pe); ret; ret = Process32Next(hSnapshot, &pe))
	{
		if (cgiStrEq(_exe.c_str(), pe.szExeFile))
		{
			printf("%s isStart\r\n", pe.szExeFile);
			bRet = true;
		}
	}
	CloseHandle(hSnapshot);
	return bRet;
}

int Ut_getPid(std::string _exe)
{
	int pid = -1;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot) {
		printf("��ȡ�����б�ʧ��");
		return pid;
	}
	PROCESSENTRY32 pe = { sizeof(pe) };
	for (BOOL ret = Process32First(hSnapshot, &pe); ret; ret = Process32Next(hSnapshot, &pe))
	{
		if (cgiStrEq(_exe.c_str(), pe.szExeFile))
		{
			printf("%s isStart\r\n", pe.szExeFile);
			pid = pe.th32ProcessID;
		}
	}
	CloseHandle(hSnapshot);
	return pid;
}

std::string Ut_getTime_Hour()
{
	time_t tt = time(NULL);//��䷵�ص�ֻ��һ��ʱ��cuo
	tm* t = localtime(&tt);
	std::string nowTime;
	int size = 6;
	std::unique_ptr<char[]> buf(new char[size]);
	sprintf(buf.get(), "%02d:%02d", t->tm_hour, t->tm_min);
	nowTime = std::string(buf.get(), buf.get() + size - 1);
	return nowTime;
}


std::string Ut_getTime_day()
{
	time_t tt = time(NULL);//��䷵�ص�ֻ��һ��ʱ��cuo
	tm* t = localtime(&tt);
	std::string nowTime;
	int size = 20;
	std::unique_ptr<char[]> buf(new char[size]);
	sprintf(buf.get(), "%d-%02d-%02d %02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	nowTime = std::string(buf.get(), buf.get() + size - 1);
	return nowTime;
}

void Ut_goSystem(std::string cmd)
{	
	printf("system %s\r\n", cmd.c_str());
	int res = system(cmd.c_str());
	printf("res %d\r\n",res);
}

bool canRestart = 1; //���û�������� ���Ա�����
#define UT_SERVER "rainbow"
#define UT_EXE UT_SERVER##".exe"
#define UT_START "net start "##UT_SERVER
#define UT_STOP "net stop "##UT_SERVER

using namespace std;
DWORD WINAPI ThreadFuncFirst(LPVOID param)
{
	while (1) {
		Sleep(1000);
		if (STOP == g_UtSystemCmd)
		{
			Ut_goSystem(UT_STOP);
			if (!Ut_isStart(UT_EXE))
			{	
				printf("%s stop success!\r\n", Ut_getTime_day().c_str());
				g_UtSystemCmd = START;  //To reStart
			}
		}
		else if (START == g_UtSystemCmd)
		{
			Ut_goSystem(UT_START);
			if (Ut_isStart(UT_EXE))
			{
				printf("%s start success!\r\n", Ut_getTime_day().c_str());
				{
					std::lock_guard<std::mutex> guard(__m_mtx);
					canRestart = 0;  //  ��ʾ�Ѿ�������������������
				}
				g_UtSystemCmd = NONE;
			}
		}
		else 
		{
		}
	}
	return 0;
}
#include <psapi.h> 
void Ut_PrintMemoryInfo(DWORD processID)
{
	HANDLE hProcess;
	::PROCESS_MEMORY_COUNTERS pmc;
	// Print the process identifier.
	//printf( "\nProcess ID: %u\n", processID );
	// Print information about the memory usage of the process.
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, processID);
	if (NULL == hProcess)
		return;
	if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
	{
		//printf( "\tPageFaultCount: %d\n", pmc.PageFaultCount );
		//printf( "\tPageFaultCount: 0xX\n", pmc.PageFaultCount );
		//printf( "\tPeakWorkingSetSize: 0xX\n",pmc.PeakWorkingSetSize );
		//�������ռ���ڴ����
		//printf( "WorkingSetSize: %dk\n",pmc.WorkingSetSize / 1024 );
		cout << "," << pmc.WorkingSetSize / 1024 << endl;
		//printf( "\tWorkingSetSize: 0xX %dk\n", pmc.WorkingSetSize ,pmc.WorkingSetSize / 1024 );

	}
	CloseHandle(hProcess);
}

int main()
{	//��ȡʱ�� �����01:00
	//�ر�Ŀ��rainbow
	//�鿴Ŀ���Ƿ�ر�
	//����Ŀ��
	//�鿴Ŀ���Ƿ�����
	//int pid = Ut_getPid(UT_EXE);
	//Ut_PrintMemoryInfo(pid);


	DWORD dwThreadID = 0;
	HANDLE handleFirst = CreateThread(NULL, 0, ThreadFuncFirst, 0, 0, &dwThreadID);
	if (!handleFirst)
	{
		cout << "create thread 1 error:" << endl;
		return -1;
	}



	while (1) //loop...
	{
		std::string  nowTime = Ut_getTime_Hour();
		if (nowTime > "23:00")  //����canRestartʱ���
		{
			canRestart = 1;
		}
		else if ("01:00" < nowTime && nowTime < "02:00")
		{
			//����ʱ���
			{
				std::lock_guard<std::mutex> guard(__m_mtx);
				if (canRestart == 0)
				{
					continue;
				}
			}
			
			if (g_UtSystemCmd == NONE)
			{
				g_UtSystemCmd = STOP;
			}
		
		}
		Sleep(1000);
	}

	WaitForSingleObject(handleFirst, INFINITE);
	CloseHandle(handleFirst);
	return 0;
}

