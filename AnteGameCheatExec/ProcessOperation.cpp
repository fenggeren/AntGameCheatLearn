#include "ProcessOperation.h"

//��������
bool EnumProcess(IN pfTypeEnumProcess fpEnum, IN void* callBackParameter)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32  stcPe32 = { 0 };
	stcPe32.dwSize = sizeof(PROCESSENTRY32);

	//1. ����һ�����̿��յľ��
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap) return false;

	//2. ͨ�����վ����ȡ��һ�����̵���Ϣ
	if (!Process32First(hProcessSnap, &stcPe32))
	{
		CloseHandle(hProcessSnap);
		return false;
	}
	//Ͷ�ݵ�һ���ص�
	if (fpEnum(callBackParameter, &stcPe32)) goto __END;

	//3. ѭ������������Ϣ
	do 
	{
		if (fpEnum(callBackParameter, &stcPe32)) goto __END;
	} while (Process32Next(hProcessSnap, &stcPe32));
	
__END:
	//4.�رվ��
	CloseHandle(hProcessSnap);

	return true;
}


//�����߳�
bool EnumThread(IN pfTypeEnumThread pfEnum, IN void* callBackParameter)
{
	HANDLE hThreadSnap;
	THREADENTRY32 stcPe32 = { 0 };
	stcPe32.dwSize = sizeof(THREADENTRY32);

	//1. ����һ�����̿��յľ��
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (INVALID_HANDLE_VALUE == hThreadSnap) return false;

	//2. ͨ�����վ����ȡ��һ�����̵���Ϣ
	if (!Thread32First(hThreadSnap, &stcPe32))
	{
		CloseHandle(hThreadSnap);
		return false;
	}
	//Ͷ�ݵ�һ���ص�
	if (pfEnum(callBackParameter, &stcPe32)) goto __END;

	//3. ѭ������������Ϣ
	do
	{
		if (pfEnum(callBackParameter, &stcPe32))  goto __END;
	} while (Thread32Next(hThreadSnap, &stcPe32));

__END:
	//4.�رվ��
	CloseHandle(hThreadSnap);

	return true;
}


bool EnumModule(IN DWORD dwPID, IN pfTypeEnumModule pfEnum, IN void* callBackParameter)
{
	HANDLE        hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
	// 1. ����һ��ģ����صĿ��վ��
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
	if (hModuleSnap == INVALID_HANDLE_VALUE)
		return false;
	// 2. ͨ��ģ����վ����ȡ��һ��ģ����Ϣ
	if (!Module32First(hModuleSnap, &me32)) {
		CloseHandle(hModuleSnap);
		return false;
	}

	// 3. ѭ����ȡģ����Ϣ
	do {
		if (pfEnum(callBackParameter, &me32)) goto __END;
	} while (Module32Next(hModuleSnap, &me32));

__END:
	// 4. �رվ�����˳�����
	CloseHandle(hModuleSnap);
	return false;

}