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


#include <iostream>
#include <algorithm>
#include <vector>

#if defined  _WIN32
	#define WINDOWS_LEAN_AND_MEAN
	#include <Windows.h>
	#include <process.h>
#else
//TODO: Put Linux headers here
#endif

const int SPEED_TESTER_ALLOCATION_SIZE = 8*1024*1024; //In bytes
enum TestErr {SUCCESSFUL, MEM_NOT_COMMITED};
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
//HANDLE hCon;
typedef unsigned int ITER_TYPE;
unsigned int intErrCount=0, strErrCount=0;
ITER_TYPE strTestProgress;
ITER_TYPE long64TestProgress;
unsigned short ITERATION_DENOMINATOR = 1000;
ITER_TYPE nITERATIONS;
bool bReportThreadExit = false;

DWORD nWritten;
CONSOLE_SCREEN_BUFFER_INFO sBufInfo;
HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
bool PrintFileVersion()
{
#if defined _WIN32
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
#else
	//TODO: Linux code to get the version of exe file.
#endif

}

BOOL IsWow64()
{
#if defined _WIN32
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
#else
	//TODO: Linux code to find out if it is a WOW64 process
#endif
}

DWORDLONG Initialize(DWORDLONG pageCount)
{
	
	std::cout << "This is a x64 executable: ";
#if defined _WIN64
	std::cout << "Yes" <<std::endl;
#else
	std::cout << "No" <<std::endl;
	std::cout << "This is a WOW64 process: ";
	if(IsWow64()) std::cout <<"Yes";
	else std::cout<< "No";
	std::cout << std::endl;
#endif
	std::cout << "\nSystem Information:\n";
#if !defined _WIN32
	//TODO: Define SYSTEM_INFO and  MEMORYSTATUSEX manually
#endif

	SYSTEM_INFO si;
	MEMORYSTATUSEX sMemoryStatus;	
	sMemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	DWORDLONG memSize;
#if defined _WIN32
	GlobalMemoryStatusEx(&sMemoryStatus);
	GetSystemInfo(&si);
#else
//TODO: Linux code to fill the created structures
#endif
	std::cout << std::endl;
	if(pageCount < 1) //Auto max mem finder
	{
		pageCount = sMemoryStatus.ullAvailPhys / si.dwPageSize / 4;
		memSize = sMemoryStatus.ullAvailPhys / 4;
		std::cout << "Auto Calculated Number Of Pages: " << pageCount << std::endl;
		
	}else 
	{
		memSize = pageCount * si.dwPageSize;
		std::cout << "Number Of Memory Pages For Test: " << pageCount << std::endl;
	}
	
	
	std::cout << "Total Physical Memory (MB): " << sMemoryStatus.ullTotalPhys / (1024*1024)<< std::endl;
	std::cout << "Available Physical Memory (MB): " << sMemoryStatus.ullAvailPhys / (1024*1024)<< std::endl;
	std::cout << "Physical Memory In Use %: " << sMemoryStatus.dwMemoryLoad << std::endl;
	std::cout << "Total Commitable Memory (MB): " << sMemoryStatus.ullTotalPageFile/ (1024*1024)<< std::endl;
	std::cout << "Available Commitable Memory (MB): " << sMemoryStatus.ullAvailPageFile/ (1024*1024)<< std::endl;
	std::cout << "Page Size (Byte): " << si.dwPageSize << std::endl;
	std::cout << "Allocation Granularity: " << si.dwAllocationGranularity << std::endl;
	std::cout << std::endl;
	std::cout << "Processor Architecture: ";
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
#if defined _WIN32
	std::cout << std::endl << "SSE2: ";
	if(IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE)) std::cout << "Yes";
	else std::cout <<"No";

	std::cout << std::endl <<"3D Now: ";
	if(IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE)) std::cout << "Yes";
	else std::cout << "No";
	
	std::cout << std::endl << "Data Execution Prevention: ";
	if(IsProcessorFeaturePresent(PF_NX_ENABLED)) std::cout << "Enabled";
	else std::cout << "Disabled";
#else

	//TODO: Linux code to output the same info as windows (optional)
#endif
	std::cout << std::endl<< "Maximum Application Address: " << si.lpMaximumApplicationAddress<< std::endl;
	std::cout <<"Minimum Application Address: " << si.lpMinimumApplicationAddress<<std::endl;
	

	std::cout << std::endl;
	std::cout << "Commit size will be " << (memSize /1024) * 4 << " KB."<<std::endl;

	return memSize;
}

DWORD WINAPI ReportProgress(LPVOID pUpdateInterval)
{
#if defined _WIN32
	DWORDLONG updateInterval= *((DWORDLONG*) pUpdateInterval);
	std::cout << "String Test Progress %:\nInteger Test Progress %:\nErrors:"; 

	char str[6];
	GetConsoleScreenBufferInfo(hConsoleOut, &sBufInfo);
	COORD posWrite = sBufInfo.dwCursorPosition;
	posWrite.X+=18;
	--posWrite.Y;
	while(!bReportThreadExit)
	{
		--posWrite.Y;
		--posWrite.X;
		_itoa(strTestProgress/(double)nITERATIONS*100, str, 10);
		WriteConsoleOutputCharacter(hConsoleOut, str, strlen(str), posWrite, &nWritten); 
		++posWrite.Y;
		++posWrite.X;
		_itoa(long64TestProgress/(double)nITERATIONS*100, str, 10);
		WriteConsoleOutputCharacter(hConsoleOut, str, strlen(str), posWrite, &nWritten); 
		posWrite.X-=17;
		posWrite.Y+=1;
		_itoa(intErrCount+strErrCount, str, 10);
		WriteConsoleOutputCharacter(hConsoleOut, str, strlen(str), posWrite, &nWritten);
		posWrite.X+=17;
		posWrite.Y-=1;
		Sleep(updateInterval ); ///100: ticks->ratio
	}
#else
	//TODO: Linux way of outpuring data into std output
#endif
	return 0;
}
DWORD WINAPI Long64Test(LPVOID pMemSize)
{
	DWORDLONG memSize = *((DWORDLONG*) pMemSize);
	
	unsigned long long *ptr64[2] = {NULL ,NULL};

	const SIZE_T SIZE = memSize/sizeof(long long);
	try{
		ptr64[0] = new unsigned long long [SIZE];
		ptr64[1] = new unsigned long long [SIZE];
	}catch(std::bad_alloc exBadAlloc){
		std::cerr << "Memory allocation failed in integer test probably due to not enough memory." << std::endl;
		if(ptr64[0] != NULL) delete[] ptr64[0];
		return 1;
	}

	SIZE_T pos;
	
	for(ITER_TYPE iter = 0; iter < (nITERATIONS); ++iter)
	{
		long64TestProgress = iter;
		restartloop:
		for(pos = 0; pos < SIZE; ++pos) 
			ptr64[0][pos]= pos; 

		for(; pos > 0; --pos) 
			ptr64[1][SIZE - pos] = ptr64[0][pos-1];
		
		for(pos = 0; pos < SIZE; ++pos)
			if(ptr64[0][pos] + ptr64[1][pos] != SIZE - 1) {++intErrCount; iter++; goto restartloop;}
		
		memcpy(ptr64[0], ptr64[1], memSize);

		for(pos = 0; pos < SIZE; ++pos) 
			ptr64[1][pos]= pos; 
			
		for(pos = 0; pos < SIZE; ++pos) 
			if(ptr64[0][pos] + ptr64[1][pos] != SIZE - 1) {++intErrCount; break;}
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

	for(ITER_TYPE iter = 0; iter < (nITERATIONS); ++iter)
	{
		strTestProgress = iter;
		CopyMemory(ptrStr[1], ptrStr[0], memSize);

		strErrCount += abs(memcmp(ptrStr[0], ptrStr[1], memSize));

		CopyMemory(ptrStr[0], ptrStr[1], memSize);

		strErrCount += abs(memcmp(ptrStr[0], ptrStr[1], memSize));
	}
	
	delete[] ptrStr[0];
	delete[] ptrStr[1];

	return 0;
}

volatile int dummy;
int MeasureSystemSpeed()
{
	
	std::vector<int> tTime;
	int *iTest = new int[SPEED_TESTER_ALLOCATION_SIZE/sizeof(int)]; //We have allocation size so we devide it by int size to see how many ints we need
	for(int testNumber=0 ; testNumber<5;++testNumber)
	{
		DWORD tStartTime = GetTickCount();
		
		for(int j = 0; j < 300; ++j)
		{
			dummy--;
			int i = 0;
			for( ;i<SPEED_TESTER_ALLOCATION_SIZE/sizeof(int); ++i)
			{
				iTest[i] = (i+dummy*j)%4000000000 + 1; 
			}
			for(; i>=0; --i)
			{
				if(i%2 == 0) dummy = iTest[i] + dummy;
				else dummy = iTest[i-1] + dummy;
			}
		}
		tTime.push_back(GetTickCount() - tStartTime);
	}
	delete[] iTest;
	std::sort(tTime.begin(),tTime.end());
#if defined _WIN32
	GetConsoleScreenBufferInfo(hConsoleOut, &sBufInfo);
	//COORD posWrite = sBufInfo.dwCursorPosition;
	SetConsoleCursorPosition(hConsoleOut, sBufInfo.dwCursorPosition);
#else
	//TODO: Linux way of printing out
#endif

	return tTime[2]; //Middle number
}

int main(int argc, char* argv[])
{
	std::cout << "FSB Data Integrity Tester by SiavoshKC. V";
	if(!PrintFileVersion())
	{
		std::cerr << "Getting file version has failed.\n";
		return 1;
	}

	std::cout << "\nCopyright 2010 under GPL 3 License. To get a copy of the license\ngo to http://www.gnu.org/licenses\n\nThis program will try to detect data corruption in transfer\nbetween CPU and Main Memory.\n";
	std::cout << "Use /pc:n switch where n is the number of memory pages to be commited\nin the test.\n" ;
	std::cout << "Use /td:l switch where l is the level of iteration from 1 to 5.\n1 will result in fastest and 5 in slowest but deepest test. Default is 2.\n" << std::endl;
	unsigned int pageCount = 1024;
	bool bTdSwitch = false;
	bool bPcSwitch = false;
	for(int i=0; i <argc;++i)
	{
		if(!_strnicmp(argv[i], "/pc:", 4))//Page Count
		{
			if(!bPcSwitch)
			{
				pageCount = atoi(&argv[i][4]);
				bPcSwitch = true;
			}
			else
				std::cout << "Switch /pc is used more than once, ignoring.\n";
		}else if (!_strnicmp(argv[i], "/td:", 4)) //Iteration Depth
		{
			if(!bTdSwitch)
			{
				switch(atoi(&argv[i][4]))
				{
				case 1:
					ITERATION_DENOMINATOR = 10000;
					std::cout << "Test Depth: 1\n";
					break;
				case 2:
					ITERATION_DENOMINATOR = 1000;
					std::cout << "Test Depth: 2\n";
					break;
				case 3:
					ITERATION_DENOMINATOR = 100;
					std::cout << "Test Depth: 3\n";
					break;
				case 4:
					ITERATION_DENOMINATOR = 10;
					std::cout << "Test Depth: 4\n";
					break;
				case 5:
					ITERATION_DENOMINATOR = 1;
					std::cout << "Test Depth: 5\n";
					break;
				default:
					std::cout << "Error: Invalid Parameter.\n";
					return 0;
				}
				bTdSwitch = true;
			}
			else
				std::cout << "Switch /td is used more than once, ignoring.\n";
		}
	}
	
	std::cout << std::endl;
	
	nITERATIONS = UINT_MAX / ITERATION_DENOMINATOR;
	DWORDLONG memSize = Initialize(pageCount);
#if defined _WIN32
	if(!SetProcessWorkingSetSize(GetCurrentProcess(), memSize*3, memSize*4))
		std::cerr << "Unable to set working size. Using windows default instead.\n";
#endif
	std::cout << "Assessing system speed...";
	int testResult = MeasureSystemSpeed();
	std::cout << "Test routine took about " <<testResult <<" milliseconds."<< std::endl;

	int updateInterval = memSize / ITERATION_DENOMINATOR * testResult / 100 * 4;
	std::cout << "Progress update interval is " << updateInterval << " milliseconds.\n";

	if(!SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS))
		std::cerr << "Unable to set priority below normal. Using normal instead. This will significantly slow down your system while test is running.\n";
	else if(pageCount>1000 && ITERATION_DENOMINATOR<100) 
		std::cout << "WARNING: This test may take several hours depending on parameters. You can work with other\nprograms while the test is running." << std::endl; 
	
	HANDLE hThread[2]; 
	
	std::cout << "Test Started...\n" ;
#if defined _WIN32
	hThread[0] = CreateThread(NULL, 0, StrTest, &memSize, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, Long64Test, &memSize, 0, NULL);
	HANDLE hReportThread = CreateThread(NULL, 0, ReportProgress, &updateInterval, 0, NULL);

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

	if(!dwStrTestExitCode || !dwULong64TestExitCode)
	{
		if(strErrCount == 0 && intErrCount == 0)
			SetConsoleTextAttribute(hConsoleOut, FOREGROUND_GREEN|FOREGROUND_INTENSITY);
		else
			SetConsoleTextAttribute(hConsoleOut, FOREGROUND_RED|FOREGROUND_INTENSITY);
		
		std::cout <<"Done with " << strErrCount << " errors in string and " << intErrCount << " errors in integer tests." << std::endl;
	}
	else return 1;

	SetConsoleTextAttribute(hConsoleOut, FOREGROUND_GREEN|FOREGROUND_BLUE| FOREGROUND_RED);
#else
	//TODO: Linux way of creating and managing threads and showing the results
#endif
	return 0;
}

