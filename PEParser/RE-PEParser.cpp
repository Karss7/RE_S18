// RE-PEParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <assert.h>

#ifdef _UNICODE
#define _tfopen _wfopen
#define _ftprintf fwprintf
#endif

int _tmain(int argc, _TCHAR* argv[])
{

	if (argc != 2)
	{
		printf("Usage: PEParser <logfile> {<image.exe>} \n");
		return 1;
	}

	FILE* log = _tfopen(argv[1], _T("wt")); //TODO switch to "at"
	_ftprintf(log, _T("PEParse started \n"));

	HANDLE hFile = CreateFile(argv[0], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	
	if (INVALID_HANDLE_VALUE == hFile)
	{
		printf("CreateFile fails with %d \n", GetLastError());
		return 2;
	}

	HANDLE hFileMapping = CreateFileMapping(hFile, NULL, SEC_IMAGE | PAGE_READONLY, 0, 0, NULL);

	if (NULL == hFileMapping)
	{
		printf("CreateFileMapping fails with %d \n", GetLastError());
		return 3;
	}

	PVOID pImageBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (nullptr /*style exception for edu purpose*/ == pImageBase)
	{
		printf("MapViewOfFile fails with %d \n", GetLastError());
		return 4;
	}

	printf("File was mapped, starts with %c%c \n", ((char*) pImageBase)[0], ((char*)pImageBase)[1]);

	PIMAGE_DOS_HEADER pImageDosHeader = (PIMAGE_DOS_HEADER)pImageBase;
	//PIMAGE_NT_HEADERS pPeHeader = (PIMAGE_NT_HEADERS)(((DWORD_PTR)(pImageBase)) + (pImageDosHeader->e_lfanew));

#define RVA_TO_VA(ptype, base, offset) \
		(ptype)(((DWORD_PTR)(base)) + (offset))

	PIMAGE_NT_HEADERS pPeHeader = RVA_TO_VA(PIMAGE_NT_HEADERS, pImageBase, pImageDosHeader->e_lfanew);

	printf("PeHeader starts with %c%c%d%d \n", ((char*)pPeHeader)[0], 
		((char*)pPeHeader)[1], ((char*)pPeHeader)[2], ((char*)pPeHeader)[3]);

	BOOL is64 = FALSE;
	switch (pPeHeader->FileHeader.Machine) 
	{
	case IMAGE_FILE_MACHINE_I386:
		assert(sizeof(IMAGE_OPTIONAL_HEADER32) == pPeHeader->FileHeader.SizeOfOptionalHeader);
		break;
	case IMAGE_FILE_MACHINE_AMD64:
		assert(sizeof(IMAGE_OPTIONAL_HEADER64) == pPeHeader->FileHeader.SizeOfOptionalHeader);
		is64 = TRUE;
		break;
	default:
		printf("Oups, unsupported machine %d\n", pPeHeader->FileHeader.Machine);
		break;
	}

	DWORD PeHeaderSize = sizeof(DWORD) + 
		sizeof(IMAGE_FILE_HEADER) + 
		(is64 ? sizeof(IMAGE_OPTIONAL_HEADER64) : sizeof(IMAGE_OPTIONAL_HEADER32));

	//TODO NEED ALIGNMENT MACROS
	assert(0 == (pImageDosHeader->e_lfanew + PeHeaderSize) % (is64 ? 8 : 4));

	PIMAGE_SECTION_HEADER pSectionHeader = RVA_TO_VA(PIMAGE_SECTION_HEADER, pImageBase, pImageDosHeader->e_lfanew + PeHeaderSize);
	for (int i = 0; i < pPeHeader->FileHeader.NumberOfSections; i++) 
	{
		char name[9];
		memcpy(name, pSectionHeader->Name, 8);
		name[8] = 0;
		printf("Section #%02d is %s\n", i, name);
		pSectionHeader++;
	}

	//IMAGE_IMPORT_DESCRIPTOR - task, read and google it
	//need list of dlls

	if (is64) 
	{
		//decide depending on platfrom
	}
	else
	{
	}

	//iterate and print dlls

	//RUNTIME_FUNCTION - also begin to read it

	fclose(log);
    return 0;
}

