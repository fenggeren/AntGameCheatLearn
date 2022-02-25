#ifndef PEOPERATION_H
#define PEOPERATION_H

#include <windows.h>

//�ж��Ƿ���PE�ļ�
BOOL IsPEFile(IN BYTE* pFileBuff);

//��ȡdosͷ
IMAGE_DOS_HEADER* GetDosHeader(IN BYTE* pFileBuff);

//��ȡNTͷ
IMAGE_NT_HEADERS* GetNtHeader(IN BYTE* pFileBuff);


//��ȡĿ¼��
IMAGE_DATA_DIRECTORY* GetDirectory(IN BYTE* pFileBuff);

// ��ȡ������
IMAGE_EXPORT_DIRECTORY* GetExportTable(IN BYTE* pFileBuff);

//��ȡ�����
IMAGE_IMPORT_DESCRIPTOR* GetImportTable(IN BYTE* pFileBuff);

//��ȡ��Դ��
IMAGE_RESOURCE_DIRECTORY* GetResourceDirectory(IN BYTE* pFileBuff);

//��ȡ�ض�λ��
IMAGE_BASE_RELOCATION* GetBaseRelocation(IN BYTE* pFileBuff);

//��ȡ��ʱ�����
IMAGE_DELAYLOAD_DESCRIPTOR* GetDelayLoadDescriptor(IN BYTE *pFileBuff);

//��ȡtls��
IMAGE_TLS_DIRECTORY* GetImageTls(IN BYTE *pFileBuff);

//�޸Ļ���ַ
BOOL ModifyImageBase(IN BYTE *pFileBuff, IN ULONG ulNewImageBase);

//����һ���ֽ�
void AddSection(
	IN BYTE *pFileBuff,   //��Ҫ���ӽڵ��ļ�������
	IN int nOldFileBuffSize,  //��Ҫ���ӽڵ��ļ���������С
	IN char* pszSectionName,  //��Ҫ���ӵĽ�����(����ǰ7���ַ���Ч)
	IN BYTE *pFillBuff, //�ڽ�����������
	IN int nFillSize,      //�������Ĵ�С
	IN DWORD dwCharacteristics, //����ӽڵ�����   IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
	OUT BYTE** newBuffOfAddress, //����½ں󻺳���
	OUT int *pNewFileSize,   //�����ӵĽڻ�������С
	OUT IMAGE_SECTION_HEADER *newSectionAddr //�����ӵķֽ���Ϣ
	);

//���÷ֽ�����
void SetSectionCharacteristics(IN BYTE *pFileBuff, IN DWORD dwCharacteristics);

//��ȡһ���ֽڵ�����
DWORD GetSectionCharacteristics(IN BYTE *pFileBuff, IN char* szSectionName);

//�ƶ��������ӷֽ�
void MoveImport(
	IN BYTE *pFileBuff, //��Ҫ�ƶ��Ļ����� 
	IN int nOldFileBuffSize, //��Ҫ�ƶ��ڵ��ļ���������С 
	IN char* pszSectionName, //��Ҫ���ӵĽ�����(����ǰ7���ַ���Ч) 
	IN DWORD dwCharacteristics, //�����ֽڵ�����
	OUT BYTE** newBuffOfAddress, //����½ں󻺳��� 
	OUT int *pNewFileSize,//�����ӵĽڻ�������С 
	OUT IMAGE_SECTION_HEADER *newSectionAddr //�����ӵķֽ���Ϣ
	);

//��ӵ������͵��뺯��
void AddImportDll(
	IN BYTE *pFileBuff,
	IN char * szDllName, //��ӵ�dll
	IN WORD wHint //�������
	);

//RVA ת����Foa
BOOL RVA2Foa(IN BYTE* pFileBuff, IN ULONG ulRva, OUT ULONG *pulFileOffset);

//Foa ת���� RVA
BOOL Foa2RVA(IN BYTE* pFileBuff, IN ULONG ulFileOffset, OUT ULONG *pulRva);


#endif