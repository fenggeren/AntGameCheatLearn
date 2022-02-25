#include "AntiCreateProcess.h"
#include <Psapi.h>
#include <iostream>
#include "PEOperation.h"


#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)


//////////////////////////////�ڲ�����///////////////////////////////////////
static DWORD s_dwSelfID = 0; //��ǰ����id
static DWORD s_dwExplorerProcessID = 0; //������id  // windows��Դ����������id
static bool s_bSuspendCreateProcess = false;

//
static bool EnumProcessFunc(IN  void *callBackParameter, IN PROCESSENTRY32* pProcessEntry)
{
	if (pProcessEntry->th32ProcessID == s_dwSelfID)
	{
		// ����ͨ������������Ŀ�ִ������������̶���explorer��Դ��������
		// �����ж��Ƿ�����Դ������������˵����������ʽ�����⡣
		if (pProcessEntry->th32ParentProcessID != s_dwExplorerProcessID) // ���ø�DLL�Ľ��̵ĸ����̲�����Դ����������
		{
			s_bSuspendCreateProcess = true;
			return true;
		}
		else
		{
			std::cout << pProcessEntry->th32ProcessID << " " << pProcessEntry->th32ParentProcessID << std::endl;
		}
	}
	return false;
}



BOOL IsProcessType(HANDLE hHandle)
{
	NTSTATUS nsQuery;
	BOOL bRet = TRUE;
	POBJECT_TYPE_INFORMATION obTypeInfo = NULL;
	obTypeInfo = (POBJECT_TYPE_INFORMATION)VirtualAlloc(NULL, sizeof(OBJECT_TYPE_INFORMATION)+0x1000, MEM_COMMIT, PAGE_READWRITE);
	nsQuery = ZwQueryObject(hHandle, ObjectTypeInformation, obTypeInfo, sizeof(OBJECT_TYPE_INFORMATION)+0x1000, NULL);
	if (NT_SUCCESS(nsQuery))
	{
		if (_wcsicmp(obTypeInfo->TypeName.Buffer, L"Process") != 0)
		{
			bRet = FALSE;
		}
	}
	VirtualFree(obTypeInfo, 0, MEM_RELEASE);
	return bRet;
}
HANDLE DumpHandle(HANDLE hProcessId, HANDLE hHandleValue)
{
	HANDLE hRet = 0;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS nsProcess;
	HANDLE hProcess;
	CLIENT_ID ProcessId = { 0 };
	ProcessId.UniqueProcess = hProcessId;
	InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
	nsProcess = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &ObjectAttributes, &ProcessId);

	if (NT_SUCCESS(nsProcess))
	{
		TCHAR szFileName[MAX_PATH] = { 0 };
		GetProcessImageFileName(hProcess, szFileName, _countof(szFileName));
		PathStripPath(szFileName);


		if (!_tcscmp(szFileName, _T("csrss.exe")) ||
			!_tcscmp(szFileName, _T("lsass.exe")) ||
			!_tcscmp(szFileName, _T("svchost.exe")) ||
			!_tcscmp(szFileName, _T("explorer.exe")))
		{
			return NULL;
		}

		NTSTATUS nsDup;
		HANDLE hLocalHandle;
		nsDup = ZwDuplicateObject(
			hProcess,
			hHandleValue,
			GetCurrentProcess(),
			&hLocalHandle,
			0L,
			0L,
			DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
			);//�ܸ��Ƴɹ��ľ�����ǿ��þ��
		if (NT_SUCCESS(nsDup))
		{
			//����׼����ѯ��
			hRet = hLocalHandle;
		}
		_tprintf(TEXT("�ҵ�һ��\n")); 
		CloseHandle(hProcess);
	}
	return hRet;
}

VOID KillHandle(HANDLE hProcessId, HANDLE hHandleValue)
{
	if (GetCurrentProcessId() == (DWORD)hProcessId)
	{
		return;
	}

	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS nsProcess;
	HANDLE hProcess;
	CLIENT_ID ProcessId = { 0 };
	ProcessId.UniqueProcess = hProcessId;
	InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
	nsProcess = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &ObjectAttributes, &ProcessId);
	if (NT_SUCCESS(nsProcess))
	{
		NTSTATUS nsDup;
		HANDLE hLocalHandle;
		nsDup = ZwDuplicateObject(
			hProcess,
			hHandleValue,
			GetCurrentProcess(),
			&hLocalHandle,
			0L,
			0L,
			DUPLICATE_CLOSE_SOURCE
			);//���ж���Kill
		if (NT_SUCCESS(nsDup))
		{
			CloseHandle(hLocalHandle);
		}
		CloseHandle(hProcess);
	}
	return;
}

// ɱ�����������ڴ���ӵ�е�->ָ�� �����̵ľ��
// �ž���������ӵ��ֻ�뱾���̵ľ��
DWORD WINAPI KillHandleThread(LPVOID lparam)
{
	while (1)
	{
		NTSTATUS ns;
		ULONG nSize = 0;
		ns = ZwQuerySystemInformation(SystemHandleInformation, NULL, 0, &nSize);
		if (ns == STATUS_INFO_LENGTH_MISMATCH)
		{
			//////////////////////////////////////////////////////////////////////////
			//����ͨ��NULL������ȡBuffer�Ĵ�С,Ȼ������Buffer
			//////////////////////////////////////////////////////////////////////////
		Loop:
			PVOID pBuffer = NULL;
			pBuffer = VirtualAlloc(NULL, nSize, MEM_COMMIT, PAGE_READWRITE);//������ں���̬������ʹ��Pool���������ں��ڴ������̬������
			if (pBuffer)
			{
				RtlZeroMemory(pBuffer, nSize);
				ns = ZwQuerySystemInformation(SystemHandleInformation, pBuffer, nSize, &nSize);
				if (NT_SUCCESS(ns))
				{  
					//////////////////////////////////////////////////////////////////////////
					//�ڶ��ε��û�ȡHandle�ɹ��ˣ����￪ʼд�����ˣ�����
					//////////////////////////////////////////////////////////////////////////
					ULONG nIndex = 0;
					PSYSTEM_HANDLE_INFORMATION pSysinfo = NULL;
					pSysinfo = (PSYSTEM_HANDLE_INFORMATION)pBuffer;
					for (nIndex = 0; nIndex < pSysinfo->NumberOfHandles; nIndex++)
					{
						if (pSysinfo->Handles[nIndex].UniqueProcessId != GetCurrentProcessId())
						{ 
							auto phandle = pSysinfo->Handles[nIndex]; 
							//���Ǳ����̵Ķ��������ȴ�Ŀ����̣�
							HANDLE hLocalHandle = DumpHandle((HANDLE)pSysinfo->Handles[nIndex].UniqueProcessId, (HANDLE)pSysinfo->Handles[nIndex].HandleValue);
							if (hLocalHandle)
							{
								if (IsProcessType(hLocalHandle))
								{ 
									TCHAR szFileName[MAX_PATH] = { 0 };
									GetProcessImageFileName(hLocalHandle, szFileName, MAX_PATH); 
									if (GetProcessId(hLocalHandle) == GetCurrentProcessId())
									{
										KillHandle((HANDLE)pSysinfo->Handles[nIndex].UniqueProcessId, (HANDLE)pSysinfo->Handles[nIndex].HandleValue); 
									} 
								}  
#if 0
								CloseHandle(hLocalHandle);
#else 
								__try
								{  
									CloseHandle(hLocalHandle);
								}
								__except (EXCEPTION_EXECUTE_HANDLER)
								{ 
									printf("EXCEPTION:   %d------ %d -------\n", __LINE__, GetLastError());
								}
#endif
							}
						}
						else
						{
							if (pSysinfo->Handles[nIndex].UniqueProcessId != GetCurrentProcessId())
							{
								if (IsProcessType((HANDLE)pSysinfo->Handles[nIndex].HandleValue))
								{
									CloseHandle((HANDLE)pSysinfo->Handles[nIndex].HandleValue);
								}
							}

						}
					}
				}
				VirtualFree(pBuffer, 0, MEM_RELEASE);//�ͷ��ڴ�
				if (ns == STATUS_INFO_LENGTH_MISMATCH)//������ڴ治����������10���������룡��
				{
					//nSize = nSize * 10;
					goto Loop;
				}
			}
		}
		Sleep(1000);
	}
	return 0;
}


//////////////////////////////����ӿ�///////////////////////////////////////
//��⸸����; �Ƿ���������
void CheckSuspendCreateProcess()
{
	//��⸸����
	s_dwSelfID = GetCurrentProcessId();
	GetWindowThreadProcessId(FindWindow(TEXT("Progman"), NULL), &s_dwExplorerProcessID);
	EnumProcess(EnumProcessFunc, NULL);
}

//������ڱ�Ľ��̵ľ��
void ClearSelfHandleInOtherProcess()
{
	CreateThread(NULL, 0, KillHandleThread, NULL, 0, NULL);
}

bool IsSuspendProcess()
{
#ifdef _DEBUG
	return true;
#else
	return s_bSuspendCreateProcess;
#endif // DEBUG

	
}


