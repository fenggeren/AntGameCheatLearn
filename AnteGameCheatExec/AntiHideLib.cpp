#include "AntiHideLib.h"
#include "AntiCheat.h"
#include "Dbg.h"
#include "ProcessOperation.h"
#include "PEOperation.h"
#include "UnDocoumentApi.h"

#include "detours/detours.h"
#pragma comment(lib,"detours/lib.X86/detours.lib")

// BegEngine��ʹ�õ�ʱ����Ҫ������
#define BEA_ENGINE_STATIC
#define BEA_USE_STDCALL
#include "BeaEngine_4.1/Win32/headers/BeaEngine.h"
#pragma comment (lib , "BeaEngine_4.1/Win32/Win32/Lib/BeaEngine.lib")
// ��ֹ�������
#pragma comment(linker, "/NODEFAULTLIB:\"crt.lib\"")



//////////////////////////////���Ͷ���///////////////////////////////////////

typedef NTSTATUS(WINAPI *fpLdrLoadDll)(
	IN PWCHAR PathToFile OPTIONAL,
	IN ULONG Flags OPTIONAL,
	IN PUNICODE_STRING ModuleFileName,
	OUT PHANDLE ModuleHandle);

//////////////////////////////�ڲ�����///////////////////////////////////////
static fpLdrLoadDll s_fpSrcLdrDll = NULL;
static bool s_bHidMode = false;   //�Ƿ񣺼��صĶ�̬���Ƿ�������ģ��Ķ���



//ɽկ�汾��ldrloaddll
static NTSTATUS WINAPI MyLdrLoadDll(
	IN PWCHAR PathToFile OPTIONAL,
	IN ULONG Flags OPTIONAL,
	IN PUNICODE_STRING ModuleFileName,
	OUT PHANDLE ModuleHandle)
{
	NTSTATUS ntStatus;
	WCHAR szDllName[MAX_PATH];
	ZeroMemory(szDllName, sizeof(szDllName));
	memcpy(szDllName, ModuleFileName->Buffer, ModuleFileName->Length);
	//�ڼ���֮ǰ�ж��¸�ģ���Ƿ񱻼��ع�
	HMODULE hPreMod = GetModuleHandleW(szDllName);
	ntStatus = s_fpSrcLdrDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
	DWORD dwLastError = GetLastError();
	//���û�б����ع�����ִ����s_fpSrcLdrDll�ɹ������Ƿ����������Լ��Ĳ���
	if (STATUS_SUCCESS == ntStatus &&NULL == hPreMod)
	{
		//GetModuleHandleW��ʵ�Ƕ�ȡpeb����Ϣ,û��Ҫ�ٱ���һ����
		HMODULE hNowMod = GetModuleHandleW(szDllName);
		if (NULL == hNowMod) s_bHidMode = true;
	}
	//�ָ�������
	SetLastError(dwLastError);
	return ntStatus;
}



//////////////////////////////����ӿ�///////////////////////////////////////
void MonitorLoadDll()
{
	HMODULE hNtDll = LoadLibrary(_T("ntdll.dll"));
	HMODULE hK32 = LoadLibrary(_T("Kernel32.dll"));
	s_fpSrcLdrDll = (fpLdrLoadDll)GetProcAddress(hNtDll, "LdrLoadDll");
	//
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((PVOID*)&s_fpSrcLdrDll, MyLdrLoadDll);
	DetourTransactionCommit();
}


bool IsFondModHidSelf()
{
	return s_bHidMode;
}
