#include "AntiCheateMain.h"
#include "GameHook.h"
#include "DriverOperation.h"
#include "HideHook.h"
#include "GameHack.h"
#include "AntiOpenMore.h"
#include "AntiApc.h"
#include "AntiCreateProcess.h"
#include "AntiHideLib.h"
#include "CheckLoop.h"
#include "AntiImm.h"

LPTOP_LEVEL_EXCEPTION_FILTER g_oldTopFilterFp = NULL; //�ɵĶ����쳣������
LONG CALLBACK UnhandleFilter(EXCEPTION_POINTERS* pException);

void AntiCheatMain()
{
#ifdef _DEBUG
	CreateDbgConsole();
#endif
	PrintDbgInfo(_T("����..."));
	BOOLEAN Old;
	RtlAdjustPrivilege(0x14, TRUE, FALSE, &Old);//��Ȩ��DEBUGȨ�ޣ�
	//���ö�����쳣���˺����Է���һ
	LPTOP_LEVEL_EXCEPTION_FILTER g_oldTopFilterFp = SetUnhandledExceptionFilter(&UnhandleFilter);
	//Ӧ�ò�����hook
	HideHook();
	CheckSuspendCreateProcess();
	//���࿪
	AntiOpenMore();

	//HookExitProcess������֪�����˳�
	OnHookExitProcess();
	//������صĺ���hook
	OnHookWnd();
	OnIsWindow();

	//�����һЩ���
	// �Ƿ������ĳЩĳ�飬���غ������ء�
	MonitorLoadDll();
	// ���APC DLLע��
	MonitorApc();

	//���뷨ע��
	MonitorImme();
	//����ѭ��
	CheckLoop();
    //������
	ClearSelfHandleInOtherProcess();
	//��������
	InitDriverCfg();
	PrintDbgInfo(_T("��ʼ��������"));
	BOOL bLoadRet = LoadDriver();
	if (!bLoadRet)
	{
		PrintDbgInfo(_T("��������ʧ��"));

		//ExitProcess(1);
	}
	PrintDbgInfo(_T("�����Ѿ�����"));
	//��������
	ConnectDriver();
	//��dll������
	GameHack();
}

LONG CALLBACK UnhandleFilter(EXCEPTION_POINTERS* pException)
{
	UnLoadDriver();
	//������ھɵĶ�����˺��������ɵĺ���
	if (NULL != g_oldTopFilterFp)
	{
		SetUnhandledExceptionFilter(g_oldTopFilterFp);
		return EXCEPTION_CONTINUE_SEARCH;
	}
	else
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
}
