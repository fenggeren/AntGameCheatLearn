#pragma once
#include "AntiCheat.h"
#include "ProcessOperation.h"

//����������ķ�ʽ��������Ȼ��shellcodeע��
void CheckSuspendCreateProcess();

//������
void ClearSelfHandleInOtherProcess();
//�ж��Ƿ��Ƿ�������ʽ����
bool IsSuspendProcess();
