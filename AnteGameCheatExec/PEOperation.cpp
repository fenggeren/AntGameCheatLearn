#include "PEOperation.h"
#include <assert.h>

BOOL IsPEFile(IN BYTE* pFileBuff)
{
	IMAGE_DOS_HEADER* pDosHeader = GetDosHeader(pFileBuff);
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
	//
	IMAGE_NT_HEADERS* pNtHeader = GetNtHeader(pFileBuff);
	if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) return FALSE;
	//
	return TRUE;
}

IMAGE_DOS_HEADER* GetDosHeader(IN BYTE* pFileBuff)
{
	IMAGE_DOS_HEADER* pDosHdr = NULL;
	return (IMAGE_DOS_HEADER*)pFileBuff;
}

IMAGE_NT_HEADERS*  GetNtHeader(IN BYTE* pFileBuff)
{
	IMAGE_DOS_HEADER *pDosHeader = GetDosHeader(pFileBuff);
	return (IMAGE_NT_HEADERS*)((ULONG_PTR)pDosHeader->e_lfanew + (ULONG_PTR)pFileBuff);
}


IMAGE_DATA_DIRECTORY*  GetDirectory(IN BYTE* pFileBuff)
{
	IMAGE_NT_HEADERS *pNtHeader = GetNtHeader(pFileBuff);
	return pNtHeader->OptionalHeader.DataDirectory;
}


IMAGE_EXPORT_DIRECTORY* GetExportTable(IN BYTE* pFileBuff)
{
	IMAGE_DATA_DIRECTORY* pDirectory = GetDirectory(pFileBuff);
	if (0 != pDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)
	{
		
		ULONG ulFoa =0;
		RVA2Foa(pFileBuff, pDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, &ulFoa);
		return (IMAGE_EXPORT_DIRECTORY*)( (ULONG_PTR)ulFoa + (ULONG_PTR)pFileBuff );
	}
	else
	{
		return NULL;
	}
}


IMAGE_IMPORT_DESCRIPTOR* GetImportTable(IN BYTE* pFileBuff)
{
	IMAGE_DATA_DIRECTORY* pDirectory = GetDirectory(pFileBuff);
	if (0 != pDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
	{
		ULONG ulFoa;
		RVA2Foa(pFileBuff, pDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, &ulFoa);
		return (IMAGE_IMPORT_DESCRIPTOR*)((ULONG_PTR)ulFoa + (ULONG_PTR)pFileBuff);
	}
	else
	{
		return NULL;
	}

}

IMAGE_RESOURCE_DIRECTORY* GetResourceDirectory(IN BYTE* pFileBuff)
{
	IMAGE_DATA_DIRECTORY* pDirectory = GetDirectory(pFileBuff);
	if (0 != pDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress)
	{
		ULONG ulFoa;
		RVA2Foa(pFileBuff, pDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress, &ulFoa);
		return (IMAGE_RESOURCE_DIRECTORY*)((ULONG_PTR)ulFoa + (ULONG_PTR)pFileBuff);
	}
	else
	{
		return NULL;
	}

}

IMAGE_BASE_RELOCATION* GetBaseRelocation(IN BYTE* pFileBuff)
{
	IMAGE_DATA_DIRECTORY* pDirectory = GetDirectory(pFileBuff);
	if (0 != pDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
	{
		ULONG ulFoa = 0;
		RVA2Foa(pFileBuff, pDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress, &ulFoa);
		return (IMAGE_BASE_RELOCATION*)((ULONG_PTR)ulFoa + (ULONG_PTR)pFileBuff);
	}
	else
	{
		return NULL;
	}
}

IMAGE_DELAYLOAD_DESCRIPTOR* GetDelayLoadDescriptor(IN BYTE *pFileBuff)
{
	IMAGE_DATA_DIRECTORY* pDirectory = GetDirectory(pFileBuff);
	if (0 != pDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress)
	{
		ULONG ulFoa = 0;
		RVA2Foa(pFileBuff, pDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress, &ulFoa);
		return (IMAGE_DELAYLOAD_DESCRIPTOR*)((ULONG_PTR)ulFoa + (ULONG_PTR)pFileBuff);
	}
	else
	{
		return NULL;
	}
}

IMAGE_TLS_DIRECTORY* GetImageTls(IN BYTE *pFileBuff)
{
	IMAGE_DATA_DIRECTORY* pDirectory = GetDirectory(pFileBuff);
	if (0 != pDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress)
	{
		ULONG ulFoa = 0;
		RVA2Foa(pFileBuff, pDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress, &ulFoa);
		return (IMAGE_TLS_DIRECTORY*)((ULONG_PTR)ulFoa + (ULONG_PTR)pFileBuff);
	}
	else
	{
		return NULL;
	}
}

BOOL ModifyImageBase(IN BYTE *pFileBuff, IN ULONG ulNewImageBase)
{

	IMAGE_BASE_RELOCATION *pRelocationTab = GetBaseRelocation(pFileBuff);
	if (NULL == pRelocationTab) return FALSE;
	//
	IMAGE_NT_HEADERS* pNtHeader = GetNtHeader(pFileBuff);
	ULONG ulOldImageBase = pNtHeader->OptionalHeader.ImageBase;
	pNtHeader->OptionalHeader.ImageBase = ulNewImageBase;
	//ִ���ض�λ
	struct TypeOffset
	{
		WORD Offset : 12;  // (1) ��СΪ12Bit���ض�λƫ��
		WORD Type : 4;    // (2) ��СΪ4Bit���ض�λ��Ϣ����ֵ
	};
	while (pRelocationTab->SizeOfBlock != 0)
	{

		TypeOffset* pTypeOffset;
		pTypeOffset = (TypeOffset*)(pRelocationTab + 1);
		ULONG ulCount = (pRelocationTab->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

		for (ULONG i = 0; i < ulCount; ++i)
		{
			if (pTypeOffset[i].Type == 3)
			{
				ULONG ulFixAddr = pRelocationTab->VirtualAddress + pTypeOffset[i].Offset;
				ULONG ulFixAddrOffset = 0;
				RVA2Foa(pFileBuff, ulFixAddr, &ulFixAddrOffset);
				//
				ULONG ulOldValue = *(ULONG_PTR*)((ULONG_PTR)pFileBuff + ulFixAddrOffset);
				ULONG ulNewValue = ulOldValue - ulOldImageBase + ulNewImageBase;
				*(ULONG_PTR*)((ULONG_PTR)pFileBuff + ulFixAddrOffset) = ulNewValue;
				//printf("��Ҫ�޸ĵĵ�ַ:RVA:0x%08X , OFS: 0x%08X\n", dwFixAddr, dwFixAddrOffset);
			}
		}
		//�¸��ض�λ���λ��
		pRelocationTab = (IMAGE_BASE_RELOCATION*)((ULONG_PTR)pRelocationTab + pRelocationTab->SizeOfBlock);
	}
	return TRUE;
}

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
	)
{
	//��ȡ�ļ�ͷ����չͷ
	IMAGE_NT_HEADERS *pNtHeader = GetNtHeader(pFileBuff);

	//������·ֽڿ�ʼλ�õ�Rva
	ULONG ulSecAlimentCnt =
		pNtHeader->OptionalHeader.SizeOfImage / pNtHeader->OptionalHeader.SectionAlignment + \
		(pNtHeader->OptionalHeader.SizeOfImage % pNtHeader->OptionalHeader.SectionAlignment ? 1 : 0);
	ULONG ulSectionBaseRva = ulSecAlimentCnt * pNtHeader->OptionalHeader.SectionAlignment;

	//�·ֽڿ�ʼλ�õ�Foa
	ULONG ulFileAlimentCnt =
		nOldFileBuffSize / pNtHeader->OptionalHeader.FileAlignment + \
		(nOldFileBuffSize % pNtHeader->OptionalHeader.FileAlignment ? 1 : 0);
	ULONG ulFileBaseFoa = ulFileAlimentCnt * pNtHeader->OptionalHeader.FileAlignment;

	//������µ�SizeOfRawData
	ULONG ulFillSizeofRawDataCnt =
		nFillSize / pNtHeader->OptionalHeader.FileAlignment + \
		(nFillSize % pNtHeader->OptionalHeader.FileAlignment ? 1 : 0);
	ULONG ulNewSizeofRawData = ulFillSizeofRawDataCnt * pNtHeader->OptionalHeader.FileAlignment;

	
	//Ϊ�����ķֽ�����һ���ṹ�� IMAGE_SECTION_HEADER
	IMAGE_SECTION_HEADER newSection;
	ZeroMemory(&newSection, sizeof(newSection));
	memcpy(newSection.Name, pszSectionName, 7);
	newSection.Misc.VirtualSize = ulNewSizeofRawData;
	newSection.VirtualAddress = ulSectionBaseRva;
	newSection.SizeOfRawData = ulNewSizeofRawData;
	newSection.PointerToRawData = ulFileBaseFoa;
	newSection.Characteristics = dwCharacteristics;
	//�ѹ����section������ȥ
	memcpy(newSectionAddr, &newSection, sizeof(newSection));


	//������µĻ�������С
	ULONG ulNewFileBuffSize = ulFileBaseFoa + ulNewSizeofRawData;
	*newBuffOfAddress = new BYTE[ulNewFileBuffSize];
	*pNewFileSize = ulNewFileBuffSize;
	ZeroMemory(*newBuffOfAddress, *pNewFileSize);

	//�����ǰ�Ļ�����
	memcpy(*newBuffOfAddress, pFileBuff, nOldFileBuffSize);
	/*
		���������Ϣ���޸ķֽ�������imagesize
	*/

	IMAGE_NT_HEADERS *pNewNtHeader = GetNtHeader(*newBuffOfAddress);
	//�ҵ��ֽڽ����ĵط�,ֱ�Ӱѹ����IMAGE_SECTION_HEADER�ṹ������ȥ
	IMAGE_SECTION_HEADER* pSectionHeader = NULL;
	pSectionHeader = IMAGE_FIRST_SECTION(pNewNtHeader);
	pSectionHeader += pNewNtHeader->FileHeader.NumberOfSections;
	memcpy(pSectionHeader, &newSection, sizeof(IMAGE_SECTION_HEADER));

	//�޸�Fileͷ�еķֽ����� 
	pNewNtHeader->FileHeader.NumberOfSections++;

	//�����µ�SizeOfImage
	ULONG ulFileSizeofSectionDataCnt =
		nFillSize / pNewNtHeader->OptionalHeader.SectionAlignment + \
		(nFillSize % pNewNtHeader->OptionalHeader.SectionAlignment ? 1 : 0);
	ULONG ulNewSizeofSectionData = ulFileSizeofSectionDataCnt * pNewNtHeader->OptionalHeader.SectionAlignment;
	pNewNtHeader->OptionalHeader.SizeOfImage = pNewNtHeader->OptionalHeader.SizeOfImage + ulNewSizeofSectionData;


	//��������ӵĽ�
	memcpy(*newBuffOfAddress + ulFileBaseFoa, pFillBuff, nFillSize);

}


void MoveImport(
	IN BYTE *pFileBuff, //��Ҫ�ƶ��Ļ����� 
	IN int nOldFileBuffSize, //��Ҫ�ƶ��ڵ��ļ���������С 
	IN char* pszSectionName, //��Ҫ���ӵĽ�����(����ǰ7���ַ���Ч) 
	IN DWORD dwCharacteristics, //�����ֽڵ�����
	OUT BYTE** newBuffOfAddress, //����½ں󻺳��� 
	OUT int *pNewFileSize,//�����ӵĽڻ�������С 
	OUT IMAGE_SECTION_HEADER *newSectionAddr //�����ӵķֽ���Ϣ
	)
{
	IMAGE_NT_HEADERS* pNtHeader = GetNtHeader(pFileBuff);
	IMAGE_DATA_DIRECTORY* pDirectory = GetDirectory(pFileBuff);

	//��ȡ��������ڷֽڵ��ڴ�����
	IMAGE_SECTION_HEADER*pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader);

	IMAGE_IMPORT_DESCRIPTOR* pImportTable = GetImportTable(pFileBuff);
	if (NULL == pImportTable) return;
	int nImportTableNum = 0;
	while (pImportTable[nImportTableNum].Name != 0)
	{
		++nImportTableNum;
	}
	int nSizeImportTable = sizeof(IMAGE_IMPORT_DESCRIPTOR)* nImportTableNum;
	ULONG ulImportFoa;
	RVA2Foa(pFileBuff, pDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, &ulImportFoa);

	//��ӷֽڣ����Ұ�ԭ���ĵ���������µķֽ���
	AddSection(
		pFileBuff,
		nOldFileBuffSize,
		pszSectionName,
		pFileBuff + ulImportFoa,
		pDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size,
		dwCharacteristics,
		newBuffOfAddress,
		pNewFileSize,
		newSectionAddr
		);

	//���ݷֽڵ���λ�ã��������õ����Ŀ¼
	pDirectory = GetDirectory(*newBuffOfAddress);
	pDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = newSectionAddr->VirtualAddress;

}

void AddImportDll(
	IN BYTE *pFileBuff,
	IN char * szDllName, //��ӵ�dll
	IN WORD wHint //�������
	)
{
	BOOL bRet = FALSE;
	IMAGE_IMPORT_DESCRIPTOR* pImportTable = GetImportTable(pFileBuff);
	if (NULL == pImportTable) return;
	//���������һ��
	while (pImportTable->Name != 0)
	{
		++pImportTable;
	}
	//��������¼ӵ�����λ��
	IMAGE_IMPORT_DESCRIPTOR* pAddImportTable = pImportTable;
	
	//��������������һ���հ׽ṹ֮���λ��,Ȼ�����õ���dll���ֺܺ�������
	pImportTable += 2;
	BYTE* pDllNamePos = (BYTE*)pImportTable;
	memcpy(pDllNamePos, szDllName, strlen(szDllName) + 1);

	BYTE* pHintPos = pDllNamePos + strlen(szDllName) + 1;
	*(DWORD*)pHintPos = wHint | 0x80000000;

	//������������ֺ͵���������rva
	ULONG ulDllNameFoa = pDllNamePos - pFileBuff;
	ULONG ulHintFoa = pHintPos - pFileBuff;
	ULONG ulDllNameRva = 0;
	ULONG ulHintRva = 0;
	bRet = Foa2RVA(pFileBuff, ulDllNameFoa, &ulDllNameRva);
	assert(bRet);
	bRet = Foa2RVA(pFileBuff, ulHintFoa, &ulHintRva);
	assert(bRet);

	//���쵼��������
	IMAGE_IMPORT_DESCRIPTOR newImportDescriptor = { 0 };
	newImportDescriptor.OriginalFirstThunk = ulHintRva;
	newImportDescriptor.Name = ulDllNameRva;
	newImportDescriptor.FirstThunk = ulHintRva;
	memcpy(pAddImportTable, &newImportDescriptor, sizeof(newImportDescriptor));
}

BOOL RVA2Foa(IN BYTE* pFileBuff, IN ULONG ulRva, OUT ULONG *pulFileOffset)
{
	if (!IsPEFile(pFileBuff)) return FALSE;
	//
	IMAGE_SECTION_HEADER* pSectionHeader = NULL;
	IMAGE_NT_HEADERS *pNtHeader = GetNtHeader(pFileBuff);
	pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader);

	for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; ++i)
	{
		ULONG ulVirtualSize = pSectionHeader[i].SizeOfRawData >pSectionHeader[i].Misc.VirtualSize ? pSectionHeader[i].SizeOfRawData : pSectionHeader[i].Misc.VirtualSize;
		if (ulRva >= pSectionHeader[i].VirtualAddress
			&& ulRva < pSectionHeader[i].VirtualAddress + ulVirtualSize)
		{
			ulRva -= pSectionHeader[i].VirtualAddress;
			ulRva += pSectionHeader[i].PointerToRawData;
			*pulFileOffset = ulRva;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Foa2RVA(IN BYTE* pFileBuff, IN ULONG ulFileOffset, OUT ULONG *pulRva)
{
	if (!IsPEFile(pFileBuff)) return FALSE;
	//
	IMAGE_SECTION_HEADER* pSectionHeader = NULL;
	IMAGE_NT_HEADERS *pNtHeader = GetNtHeader(pFileBuff);
	pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader);

	for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; ++i)
	{
		if (ulFileOffset >= pSectionHeader[i].PointerToRawData  && 
			ulFileOffset < pSectionHeader[i].PointerToRawData + pSectionHeader[i].SizeOfRawData)
		{
			ulFileOffset -= pSectionHeader[i].PointerToRawData;
			ulFileOffset += pSectionHeader[i].VirtualAddress;
			*pulRva = ulFileOffset;
			return TRUE;
		}
	}
	return FALSE;
}
