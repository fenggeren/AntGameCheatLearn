#include <windows.h>
#include <TlHelp32.h>

//�������̺���,����true��������������false��������
typedef bool(*pfTypeEnumProcess)(IN  void *callBackParameter, IN PROCESSENTRY32* pProcessEntry);
/*
	���ã���������
	������
		fpEnum ����������ʹ�õĻص�����
		callBackParameter���������̴��ݸ��ص������Ĳ���
	����ֵ���������true�����������أ�����ִ���쳣
*/
bool EnumProcess(IN pfTypeEnumProcess fpEnum, IN void* callBackParameter);


//�����̺߳���,����true��������������false��������
typedef bool(*pfTypeEnumThread)(IN  void *callBackParameter, IN THREADENTRY32* pThreadEntry);
/*
	���ã������߳�
	������
	fpEnum �������߳�ʹ�õĻص�����
	callBackParameter���������̴��ݸ��ص������Ĳ���
	����ֵ���������true�����������أ�����ִ���쳣
*/
bool EnumThread(IN pfTypeEnumThread pfEnum, IN void* callBackParameter);



//����ģ�麯��,����true��������������false��������
typedef bool(*pfTypeEnumModule)(IN  void *callBackParameter, IN MODULEENTRY32* pModuleEntry);
/*
	����:���ݽ���id�������̵�ģ����Ϣ
	������
	fpEnum ������ģ��ʹ�õĻص�����
	callBackParameter���������̴��ݸ��ص������Ĳ���
	����ֵ���������true�����������أ�����ִ���쳣
*/
bool EnumModule(IN DWORD dwPID, IN pfTypeEnumModule pfEnum, IN void* callBackParameter);