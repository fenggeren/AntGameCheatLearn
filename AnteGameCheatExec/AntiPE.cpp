#include "AntiPE.h"

#include "ProcessOperation.h"
#include "PEOperation.h"
#include <windows.h>
#include <tchar.h>
#include "Dbg.h"
#include <string>
#include <set>
#include <map>
#include <vector>
#include "Crypto.h"
#include "Dbg.h"
#include "FileVersionInfo.h"

#pragma comment(lib, "Version.lib")

//////////////////////////////���Ͷ���///////////////////////////////////////
#ifdef _UNICODE
#define  String std::wstring
#else
#define  String std::string
#endif

//�����е�ģ���md5��Ϣ
typedef struct
{
	String strDllPath;
	String strMd5;
	String strVersion;
}ModInfo;


//ģ����Ϣ
static std::vector<ModInfo> s_vecMd5Info;
static bool s_bHackDll = false;  //�ж��Ƿ���dll�ٳ�
TCHAR g_szLeftDll[MAX_PATH] = { 0 };
TCHAR g_szRightDll[MAX_PATH] = { 0 };


//////////////////////////////�ڲ�����///////////////////////////////////////
//�ļ��汾��Ϣ
void GetFileVersion(LPCTSTR lpFilePath, TCHAR* szVersionBuff, int nccBytes)
{
	DWORD dwHandle = 0;
	BYTE* pInfoData = NULL;
	ZeroMemory(szVersionBuff, sizeof(TCHAR)* nccBytes);
	//
	DWORD dwInfoSize = GetFileVersionInfoSize(lpFilePath, &dwHandle);
	if (dwInfoSize > 0)
	{
		pInfoData = new BYTE[dwInfoSize];
		memset(pInfoData, 0, dwInfoSize);
		if (GetFileVersionInfo(lpFilePath, dwHandle, dwInfoSize, pInfoData))
		{
			VS_FIXEDFILEINFO* vsFileInfo = NULL;
			UINT uSize = 0;
			if (VerQueryValue(pInfoData, _T("\\"), (LPVOID*)&vsFileInfo, &uSize))
			{
				_stprintf_s(szVersionBuff,
					nccBytes, _T("%d.%d.%d.%d"),
					HIWORD(vsFileInfo->dwFileVersionMS),
					LOWORD(vsFileInfo->dwFileVersionMS),
					HIWORD(vsFileInfo->dwFileVersionLS),
					LOWORD(vsFileInfo->dwFileVersionLS));
			}
		}
		if (pInfoData)
		{
			delete[] pInfoData;
			pInfoData = NULL;
		}
	}
}

//���ư�ļ����ļ�md5���Ż���Ч��
bool CalcFileMD5(IN LPCTSTR lpFilePath, OUT TCHAR* szMd5, IN int nccMd5BuffLen)
{
	bool bRet = true;
	HANDLE hFile = NULL;
	Botan::SecureVector<unsigned char> resultMd5;

	//���ļ��ں˶���
	hFile = CreateFile(lpFilePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		bRet = false;
		goto _EXIT;
	}
	DWORD	dwLowSize = GetFileSize(hFile, NULL);

	//����crcֵ
	DWORD dwCount = 0;
	DWORD dwFileSize = dwLowSize;
	BYTE totalBuff[512] = { 0 };
	BYTE perReadBuff[512];
	while (dwCount < dwFileSize)
	{
		DWORD dwRealRead = 0;
		ReadFile(hFile, perReadBuff, sizeof(perReadBuff), &dwRealRead, NULL);
		for (DWORD i = 0; i < dwRealRead; i++)
		{
			if (i % 2 == 0)
			{
				totalBuff[i] ^= perReadBuff[i];
			}
			else
			{
				totalBuff[i] += perReadBuff[i];
			}
		}
		dwCount += dwRealRead;
	}

	//����crc��md5
	Md5Hash((const char*)totalBuff, sizeof(totalBuff), resultMd5);

	TCHAR pMd5OutBuff[MAX_PATH] = { 0 };
	TCHAR szToaBuff[MAX_PATH];
	for (size_t i = 0; i < resultMd5.size(); i++)
	{
		memset(szToaBuff, 0, sizeof(szToaBuff));
		if (resultMd5[i] < 0x10)
		{
			_itot_s(0, szToaBuff, _countof(szToaBuff), 16);
			_itot_s(resultMd5[i], szToaBuff + 1, _countof(szToaBuff) - 1, 16);
		}
		else
		{
			_itot_s(resultMd5[i], szToaBuff, _countof(szToaBuff), 16);
		}
		_stprintf_s(pMd5OutBuff, _countof(pMd5OutBuff), _T("%s%s"), pMd5OutBuff, szToaBuff);
	}
	_tcscpy_s(szMd5, nccMd5BuffLen, pMd5OutBuff);

_EXIT:
	if (NULL != hFile) CloseHandle(hFile);
	return bRet;
}

//����ģ�麯��,����true��������������false��������
static bool DoCalcModMD5(IN  void *callBackParameter, IN MODULEENTRY32* pModuleEntry)
{
	//PrintDbgInfo(_T("%s"), pModuleEntry->szModule);
	ModInfo modMd5Info;
	TCHAR szMD5Buff[MAX_PATH] = { 0 };
	//�����ļ���md5��Ϣ
	CalcFileMD5(pModuleEntry->szExePath, szMD5Buff, _countof(szMD5Buff));
	//��ȡ�ļ��İ汾��Ϣ
	TCHAR szVersion[MAX_PATH] = { 0 };
	GetFileVersion(pModuleEntry->szExePath, szVersion, _countof(szVersion));
	modMd5Info.strDllPath = pModuleEntry->szExePath;
	modMd5Info.strMd5 = szMD5Buff;
	modMd5Info.strVersion = szVersion;
	s_vecMd5Info.push_back(modMd5Info);
	return false;
}

//dll�ٳ��ж�
void CheckRepeatExportTable()
{
	typedef struct __NeedCmpSt
	{
		String strDllPath;
		PBYTE pFileBuff;
		bool operator < (const __NeedCmpSt &a) const
		{
			return pFileBuff < a.pFileBuff;
		}
	}NeedCmpSt;
	std::map<NeedCmpSt, NeedCmpSt> mapNeedCmp;
	std::map<String, String> mapNeedCmpMod;
	//��������ѡ���ҳ���Ҫ�Ƚϵ�ģ��
	for (size_t i = 0; i < s_vecMd5Info.size(); i++)
	{
		DWORD dwUnuse = 0;
		DWORD dwLeftFileSize = 0;
		HANDLE hLeftFile = NULL;
		BYTE *pLeftBuff = NULL;
		IMAGE_EXPORT_DIRECTORY* pLeftExport = NULL;
		bool biHit = false; //i�Ƿ���������ѡ
		//
		hLeftFile = CreateFile(s_vecMd5Info[i].strDllPath.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == hLeftFile) continue;
		dwLeftFileSize = GetFileSize(hLeftFile, &dwUnuse);
		pLeftBuff = new BYTE[dwLeftFileSize];
		ReadFile(hLeftFile, pLeftBuff, dwLeftFileSize, &dwUnuse, NULL);
		pLeftExport = GetExportTable(pLeftBuff);
		if (NULL == pLeftExport) goto __LEFT_EXIT;
		if (pLeftExport->NumberOfFunctions <= 0) goto __LEFT_EXIT;
		if (pLeftExport->NumberOfNames <= 0) goto __LEFT_EXIT;

		//i���������еĽ��бȽ�
		for (DWORD j = 0; j < s_vecMd5Info.size(); j++)
		{
			DWORD dwRightFileSize = 0;
			HANDLE hRightFile = NULL;
			BYTE *pRightBuff = NULL;
			IMAGE_EXPORT_DIRECTORY* pRightExport = NULL;
			bool bjHit = false; //j�Ƿ���������ѡ
			if (s_vecMd5Info[i].strDllPath == s_vecMd5Info[j].strDllPath) continue;
			//
			hRightFile = CreateFile(s_vecMd5Info[j].strDllPath.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);
			if (INVALID_HANDLE_VALUE == hRightFile) continue;
			dwRightFileSize = GetFileSize(hRightFile, &dwUnuse);
			pRightBuff = new BYTE[dwRightFileSize];
			ReadFile(hRightFile, pRightBuff, dwRightFileSize, &dwUnuse, NULL);
			pRightExport = GetExportTable(pRightBuff);
			if (NULL == pRightExport) goto __RIGHT_EXIT;
			if (pRightExport->NumberOfFunctions <= 0) goto __RIGHT_EXIT;
			if (pRightExport->NumberOfNames <= 0) goto __RIGHT_EXIT;

			if (pLeftExport->NumberOfFunctions == pRightExport->NumberOfFunctions ||
				pLeftExport->NumberOfNames == pRightExport->NumberOfNames)
			{
				//��ȡ�ұߵ���ߵ�ӳ����ڹ�
				auto ite = mapNeedCmpMod.find(s_vecMd5Info[j].strDllPath);
				//�����������ڴ��ұߵ���ߵ�ӳ��
				if (ite == mapNeedCmpMod.end())
				{
					//PrintDbgInfo(_T("Left:%s"), s_vecMd5Info[i].strDllPath.c_str());
					//PrintDbgInfo(_T("Right:%s"), s_vecMd5Info[j].strDllPath.c_str());
					mapNeedCmpMod.insert({ s_vecMd5Info[i].strDllPath.c_str(), s_vecMd5Info[j].strDllPath.c_str() });
					NeedCmpSt leftSt = { s_vecMd5Info[i].strDllPath.c_str(), pLeftBuff };
					NeedCmpSt rightSt = { s_vecMd5Info[j].strDllPath.c_str(), pRightBuff };
					mapNeedCmp.insert({ leftSt, rightSt });
					bjHit = true;
					biHit = true;
				}
				else
				{
					String strLeftModName = ite->second;
					//������ڴ��ұߵ���ߵ�ӳ�䣬����ߵ�ӳ�䲻�ǵ�i��ģ��
					if (strLeftModName != s_vecMd5Info[i].strDllPath)
					{
						//PrintDbgInfo(_T("Left:%s"), s_vecMd5Info[i].strDllPath.c_str());
						//PrintDbgInfo(_T("Right:%s"), s_vecMd5Info[j].strDllPath.c_str());
						mapNeedCmpMod.insert({ s_vecMd5Info[i].strDllPath.c_str(), s_vecMd5Info[j].strDllPath.c_str() });
						NeedCmpSt leftSt = { s_vecMd5Info[i].strDllPath.c_str(), pLeftBuff };
						NeedCmpSt rightSt = { s_vecMd5Info[j].strDllPath.c_str(), pRightBuff };
						mapNeedCmp.insert({ leftSt, rightSt });
						bjHit = true;
						biHit = true;
					}
				}

			}
		__RIGHT_EXIT:
			if (NULL != hRightFile) CloseHandle(hRightFile);
			if (NULL != pRightBuff && !bjHit) delete[] pRightBuff;
		}
	__LEFT_EXIT:
		if (NULL != hLeftFile) CloseHandle(hLeftFile);
		if (NULL != pLeftBuff && !biHit) delete[] pLeftBuff;
	}

	//�������ıȽ�
	std::set<PBYTE> setBuff;  //����Ψһ�ԣ�Ϊ�����������ıȽϺ��ͷ��ڴ�
	for (auto &mapIte : mapNeedCmp)
	{
		NeedCmpSt leftSt = mapIte.first;
		NeedCmpSt rightSt = mapIte.second;
		PBYTE pLeftBuff = leftSt.pFileBuff;
		PBYTE pRightBuff = rightSt.pFileBuff;
		IMAGE_EXPORT_DIRECTORY* pLeftExport = NULL;
		IMAGE_EXPORT_DIRECTORY* pRightExport = NULL;
		DWORD  dwLeftAddressOfNamesFoa = 0;
		DWORD  dwRightAddressOfNamesFoa = 0;
		pLeftExport = GetExportTable(pLeftBuff);
		pRightExport = GetExportTable(pRightBuff);
		RVA2Foa(pLeftBuff, pLeftExport->AddressOfNames, &dwLeftAddressOfNamesFoa);
		RVA2Foa(pRightBuff, pRightExport->AddressOfNames, &dwRightAddressOfNamesFoa);

		PULONG_PTR upLeftAddressOfName = (PULONG_PTR)(pLeftBuff + dwLeftAddressOfNamesFoa);
		PULONG_PTR upRightAddressOfName = (PULONG_PTR)(pRightBuff + dwRightAddressOfNamesFoa);

		DWORD i = 0;
		for (; i < pRightExport->NumberOfNames; i++)
		{
			DWORD dwLeftNameFoa = 0;
			DWORD dwRightNameFoa = 0;
			RVA2Foa(pLeftBuff, upLeftAddressOfName[i], &dwLeftNameFoa);
			RVA2Foa(pRightBuff, upRightAddressOfName[i], &dwRightNameFoa);

			char* pLeftFuncName = (char*)(pLeftBuff + dwLeftNameFoa);
			char* pRightFuncName = (char*)(pRightBuff + dwRightNameFoa);
			if (strcmp(pLeftFuncName, pRightFuncName) != 0)
				break;
		}
		if (i == pRightExport->NumberOfNames)
		{
			TCHAR szLeftVerstion[MAX_PATH] = { 0 };
			TCHAR szRightVersion[MAX_PATH] = { 0 };
			GetFileVersion(leftSt.strDllPath.c_str(), szLeftVerstion, _countof(szLeftVerstion));
			GetFileVersion(rightSt.strDllPath.c_str(), szRightVersion, _countof(szRightVersion));
			if (_tcscmp(szLeftVerstion, _T("")) == 0 ||
				_tcscmp(szRightVersion, _T("")) == 0 ||
				_tcscmp(szLeftVerstion, _T("1.0.0.1")) == 0 ||
				_tcscmp(szRightVersion, _T("1.0.0.1")) == 0)
			{
				s_bHackDll = true;
				_tcscpy_s(g_szLeftDll, _countof(g_szLeftDll), leftSt.strDllPath.c_str());
				_tcscpy_s(g_szRightDll, _countof(g_szRightDll), rightSt.strDllPath.c_str());
			}

		}

		//
		setBuff.insert(pLeftBuff);
		setBuff.insert(pRightBuff);
	}

	//�ͷ���Դ
	for (auto &setIte : setBuff)
	{
		delete[] setIte;
	}
}



//////////////////////////////����ӿ�///////////////////////////////////////
//���ģ���pe��Ϣ
void CheckModPEInfo()
{
	s_vecMd5Info.clear();
	DWORD dwSelfPID = GetCurrentProcessId();
	EnumModule(dwSelfPID, DoCalcModMD5, NULL);
	CheckRepeatExportTable();
}

bool IsFondHackDll()
{
	if (s_bHackDll)
	{
		CFileVersionInfo fileLeftV(g_szLeftDll);
		CFileVersionInfo fileRightV(g_szRightDll);
		PrintDbgInfo(_T("��˾%s || dll��%s, ��Ҫ�ϴ���������"), fileLeftV.GetCompanyName().c_str(), g_szLeftDll);
		PrintDbgInfo(_T("��˾%s || dll��%s, ��Ҫ�ϴ���������"), fileRightV.GetCompanyName().c_str(), g_szRightDll);
	}
	return true;
}

