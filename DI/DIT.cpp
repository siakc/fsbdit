//FSB Data Integrity Tester
//Copyright (C) 2010  SiavoshKC

//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <http://www.gnu.org/licenses/>.

// DI.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <process.h>
enum TestErr {SUCCESSFUL, MEM_NOT_COMMITED};
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
//HANDLE hCon;
unsigned int errCount=0;
unsigned short strTestProgress;
unsigned short long64TestProgress;
bool bReportThreadExit = false;
bool GetFileVersion()
{
	DWORD  versioninfo_size;
	void*  versioninfo;
	UINT   cchLength;
	DWORD  dwDummy;
	LPTSTR file_version;
	TCHAR  szModulePath[MAX_PATH];

	/* Get the complete path to the module(DLL or EXE) we are executing in. */
	if (0 == GetModuleFileName(NULL, szModulePath, MAX_PATH))
		return false;

	/* Get the size of the version information resource in the file. */
	versioninfo_size = GetFileVersionInfoSize(szModulePath, &dwDummy);
	if (0 == versioninfo_size)
		return false;

	/* Allocate memory for the version information resource. */
	versioninfo = malloc(versioninfo_size);
	if (NULL == versioninfo)
		return false;

	/* Get the version information resource in our buffer. */
	if (!GetFileVersionInfo(szModulePath, 0, versioninfo_size, versioninfo))
	{
		free(versioninfo);
		return false;
	}

	/* Get a pointer to the FileVersion string value in file_version. */
	if (!VerQueryValue(versioninfo,"\\StringFileInfo\\040904b0\\FileVersion", (void **) &file_version, &cchLength) ||
	    0 == cchLength || NULL == file_version)
	{
		free(versioninfo);
		return false;
	}
	std::cout << file_version;
	free(versioninfo);
	return true;
}

BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;
	LPFN_ISWOW64PROCESS fnIsWow64Process;
    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");
  
    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
			std::cerr << "IsWow64Process in kernel32.dll failed" << std::endl;
        }
    }
    return bIsWow64;
}

DWORDLONG Initialize(int pageCount)
{
	std::cout << "\nSystem Information:\n";
	std::cout << "Is this program compiled in 64 bit mode: ";
#if defined _WIN64
	std::cout << "Yes" <<std::endl;
#else
	std::cout << "No" <<std::endl;
	std::cout << "Is this WOW64 process: ";
	if(IsWow64()) std::cout <<"Yes";
	else std::cout<< "No";
	std::cout << std::endl;
#endif
	
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	MEMORYSTATUSEX sMemoryStatus;	
	sMemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&sMemoryStatus);
	DWORDLONG memSize;

	std::cout << std::endl;
	if(pageCount < 1) //Auto max mem finder
	{
		pageCount = sMemoryStatus.ullAvailPhys / si.dwPageSize / 4;
		memSize = sMemoryStatus.ullAvailPhys / 4;
		std::cout << "Auto calculated number of pages: " << pageCount << std::endl;
		
	}else 
	{
		memSize = pageCount * si.dwPageSize;
		std::cout << "Number of pages: " << pageCount << std::endl;
	}
	
	
	std::cout << "Total Physical Memory (MB): " << sMemoryStatus.ullTotalPhys / (1024*1024)<< std::endl;
	std::cout << "Available Physical Memory (MB): " << sMemoryStatus.ullAvailPhys / (1024*1024)<< std::endl;
	std::cout << "Percentage of In Use Physical Memory: " << sMemoryStatus.dwMemoryLoad << std::endl;
	std::cout << "Total Commitable Memory (MB): " << sMemoryStatus.ullTotalPageFile/ (1024*1024)<< std::endl;
	std::cout << "Available Commitable Memory (MB): " << sMemoryStatus.ullAvailPageFile/ (1024*1024)<< std::endl;
	std::cout << "Page size is " << si.dwPageSize << " bytes."<<std::endl;
	std::cout << "Allocation Granularity: " << si.dwAllocationGranularity << std::endl;
	std::cout << std::endl;
	std::cout << "Processor Architecture is: ";
	switch(si.wProcessorArchitecture){
		case PROCESSOR_ARCHITECTURE_AMD64:
			std::cout << "x64";
			break;
		case PROCESSOR_ARCHITECTURE_IA64:
			std::cout << "Intel Itanium Processor Family";
			break;
		case PROCESSOR_ARCHITECTURE_INTEL:
			std::cout << "x86";
			break;
		case PROCESSOR_ARCHITECTURE_UNKNOWN:
			std::cout << "UNKNOWN";
			break;
	}

	std::cout << std::endl << "Processor Revision (xxyy-> Model xx, Stepping yy): " << si.wProcessorRevision;
	
	std::cout << std::endl<< "PAE: ";
	if(IsProcessorFeaturePresent(PF_PAE_ENABLED)) std::cout << "Enabled";
	else std::cout << "Disabled";
	
	std::cout << std::endl << "SSE2: ";
	if(IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE)) std::cout << "Yes";
	else std::cout <<"No";

	std::cout << std::endl <<"3D Now: ";
	if(IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE)) std::cout << "Yes";
	else std::cout << "No";
	
	std::cout << std::endl << "Data Execution Prevention: ";
	if(IsProcessorFeaturePresent(PF_NX_ENABLED)) std::cout << "Enabled";
	else std::cout << "Disabled";

	std::cout << std::endl<< "Maximum Application Address: " << si.lpMaximumApplicationAddress<< std::endl;
	std::cout <<"Minimum Application Address: " << si.lpMinimumApplicationAddress<<std::endl;
	

	std::cout << std::endl;
	std::cout << "Commit size will be " << (memSize /1024) * 4 << " KB."<<std::endl;

	return memSize;
}

DWORD WINAPI ReportProgress(LPVOID pMemSize)
{
	DWORDLONG memSize= *((DWORDLONG*) pMemSize);
	std::cout << "String Test Progress %:\nInteger Test Progress %:"; 
	CONSOLE_SCREEN_BUFFER_INFO sBufInfo;
	DWORD nWritten;
	char strProgressPercent[6];
	HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsoleOut, &sBufInfo);
	COORD posWrite = sBufInfo.dwCursorPosition;
	++posWrite.X;
	while(!bReportThreadExit)
	{
		--posWrite.Y;
		--posWrite.X;
		_itoa((strTestProgress*100/USHRT_MAX), strProgressPercent, 10);
		WriteConsoleOutputCharacter(hConsoleOut, strProgressPercent, strlen(strProgressPercent), posWrite, &nWritten); 
		++posWrite.Y;
		++posWrite.X;
		_itoa((long64TestProgress*100/USHRT_MAX), strProgressPercent, 10);
		WriteConsoleOutputCharacter(hConsoleOut, strProgressPercent, strlen(strProgressPercent), posWrite, &nWritten); 
		Sleep(memSize/10);
	}

	return 0;
}
DWORD WINAPI Long64Test(LPVOID pMemSize)
{
	DWORDLONG memSize = *((DWORDLONG*) pMemSize);
	
	long long *ptr64[2] = {NULL ,NULL};

	const SIZE_T SIZE = memSize/sizeof(long long);
	try{
		ptr64[0] = new long long [SIZE];
		ptr64[1] = new long long [SIZE];
	}catch(std::bad_alloc exBadAlloc){
		std::cerr << "Memory allocation failed in integer test probably due not enough memory." << std::endl;
		if(ptr64[0] != NULL) delete[] ptr64[0];
		return 1;
	}

	SIZE_T pos;
	
	for(unsigned short iter = 0; iter < USHRT_MAX; ++iter)
	{
		long64TestProgress = iter;
		restartloop:
		for(pos = 0; pos < SIZE; ++pos) 
			ptr64[0][pos]= pos; 

		for(; pos > 0; --pos) 
			ptr64[1][SIZE - pos] = ptr64[0][pos-1];
		
		for(pos = 0; pos < SIZE; ++pos)
			if(ptr64[0][pos] + ptr64[1][pos] != SIZE - 1) {++errCount; iter++; goto restartloop;}
		
		memcpy(ptr64[0], ptr64[1], memSize);

		for(pos = 0; pos < SIZE; ++pos) 
			ptr64[1][pos]= pos; 
			
		for(pos = 0; pos < SIZE; ++pos) 
			if(ptr64[0][pos] + ptr64[1][pos] != SIZE - 1) {++errCount; break;}
	}


	delete[] ptr64[0]; 
	delete[] ptr64[1];
	return 0;
}

DWORD WINAPI StrTest(LPVOID pMemSize)
{
	DWORDLONG memSize= *((DWORDLONG*) pMemSize);
	char str[100];
	str[0] = '\0';

	LPSTR ptrStr[2] = {NULL ,NULL};
	try{
		ptrStr[0] = new char[memSize];
		ptrStr[1] = new char[memSize];
	}catch(std::bad_alloc exBadAlloc){
		std::cerr << "Memory allocation failed in string test probably due not enough memory." << std::endl;
		if(ptrStr[0] != NULL) delete[] ptrStr[0];
		return 1;
	}

	for(unsigned short iter = 0; iter < USHRT_MAX; ++iter)
	{
		strTestProgress = iter;
		CopyMemory(ptrStr[1], ptrStr[0], memSize);

		errCount += abs(memcmp(ptrStr[0], ptrStr[1], memSize));

		CopyMemory(ptrStr[0], ptrStr[1], memSize);

		errCount += abs(memcmp(ptrStr[0], ptrStr[1], memSize));
	}
	
	delete[] ptrStr[0];
	delete[] ptrStr[1];

	return 0;
}

int main(int argc, char* argv[])
{
	std::cout << "FSB Data Integrity Tester by SiavoshKC. V";
	if(!GetFileVersion())
	{
		std::cerr << "Getting file version failed.\n";
		return 1;
	}

	std::cout << "\nCopyright 2010 under GPL 3 License. To get a copy of the license\ngo to http://www.gnu.org/licenses\n\nThis program will try to detect data corruption in transfer\nfrom CPU to Main Memory and back.\n";
	std::cout << "Use /pc:n switch where n is the number of memory pages to be commited\nin the test." << std::endl;
	unsigned int pageCount = 1024;
	for(int i=0; i <argc;++i) { //Ignoring any command line arguments other than "/pc"
		if(!_strnicmp(argv[i], "/pc:", 4)){
			pageCount = atoi(&argv[i][4]);
			break;
		}
	}

	std::cout << std::endl;


	DWORDLONG memSize = Initialize(pageCount);
	if(!SetProcessWorkingSetSize(GetCurrentProcess(), memSize*3, memSize*4))
		std::cerr << "Unable to set working size, using windows default.\n";

	if(!SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS))
		std::cerr << "Unable to set priority below normal. Using normal.\n";
	else if(pageCount>100) 
		std::cout << "WARNING: This test may take several hours. You can work with other\nprograms while the test is running." << std::endl; 
	
	HANDLE hThread[2]; 
	
	std::cout << "Test Started...\n" ;
	hThread[0] = CreateThread(NULL, 0, StrTest, &memSize, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, Long64Test, &memSize, 0, NULL);
	HANDLE hReportThread = CreateThread(NULL, 0, ReportProgress, &memSize, 0, NULL);

	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	bReportThreadExit = true;
	std::cout << std::endl; 
	DWORD dwStrTestExitCode, dwULong64TestExitCode;
	if(!GetExitCodeThread(hThread[0], &dwStrTestExitCode)) {
		std::cerr << "GetExitCodeThread failed." << std::endl;
		return 1;
	}else if(dwStrTestExitCode != 0){
		std::cerr << "\nString test failed to start\n";
	}
		
	if(!GetExitCodeThread(hThread[1], &dwULong64TestExitCode)){
		std::cerr << "GetExitCodeThread failed." << std::endl;
		return 1;
	}else if(dwULong64TestExitCode != 0){
		std::cerr << "\nInteger test failed to start\n";
	}

	if(!dwStrTestExitCode ||!dwULong64TestExitCode)
		std::cout <<"Done with " << errCount<<" errors."<<std::endl;
	else return 1;


	return 0;
}

