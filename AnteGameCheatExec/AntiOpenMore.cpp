#include "AntiOpenMore.h"
#include "AntiCheat.h"

// ���û�����
void AntiOpenMore()
{
	HANDLE hMutex = NULL;
	hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, _T("llkmutex"));
	if (NULL == hMutex)
	{
		CreateMutex(NULL, TRUE, _T("llkmutex"));
	}
	else
	{
		CloseHandle(hMutex);
		MessageBox(NULL, _T("�ó�������࿪"), _T("��ʾ"), MB_OK);
		ExitProcess(1);
	}
}
