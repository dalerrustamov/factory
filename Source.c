//Assign5_Daler
#define WIN32_LEAN_AND_MEAN 
#define NOATOM
#define NOCLIPBOARD
#define NOCOMM
#define NOCTLMGR
#define NOCOLOR
#define NODEFERWINDOWPOS
#define NODESKTOP
#define NODRAWTEXT
#define NOEXTAPI
#define NOGDICAPMASKS
#define NOHELP
#define NOICONS
#define NOTIME
#define NOIMM
#define NOKANJI
#define NOKERNEL
#define NOKEYSTATES
#define NOMCX
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMSG
#define NONCMESSAGES
#define NOPROFILER
#define NORASTEROPS
#define NORESOURCE
#define NOSCROLL
#define NOSHOWWINDOW
#define NOSOUND
#define NOSYSCOMMANDS
#define NOSYSMETRICS
#define NOSYSPARAMS
#define NOTEXTMETRIC
#define NOVIRTUALKEYCODES
#define NOWH
#define NOWINDOWSTATION
#define NOWINMESSAGES
#define NOWINOFFSETS
#define NOWINSTYLES
#define OEMRESOURCE
#pragma warning(disable : 4996)

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <WinSock2.h>
#include "EncoderDecoder.h" // the header


#if !defined(_Wp64)
#define DWORD_PTR DWORD
#define LONG_PTR LONG
#define INT_PTR INT
#endif

typedef struct _Warehouse { //Each storage item will hold (2 characters and their location in the file)
	int Location;
	char Items[2];
} WAREHOUSE;

HANDLE OpenSpots; // handle for counting semaphore
HANDLE FilledSpots; // handle for counting semaphore
HANDLE InWarehouse; // handle for binary semaphore
HANDLE WritingFiles; // handle for binary semaphore

char *EncryptType; //a vaiable to hold E or D from command line, declaring globally so I can use in consumer thread

int Front = 0;
int Back = 0;
int Count = 0;
int FileLocation = 0;

void WINAPI Producer(char*);
void WINAPI Consumer(char*);

LARGE_INTEGER FileSize; //for the org.txt size

BOOL ProducerAlive = FALSE; //to check if the producers are alive
WAREHOUSE StorageItem[10]; //can store 10 storage item


int _tmain(int argc, LPTSTR argv[])
{

	InitializeEncoderDecoder(); // This is called once for setup

	HANDLE cThreads; //consumer threads 
	HANDLE pThread;  //producer thread 

	EncryptType = argv[3];

	cThreads = malloc((argc - 1) * sizeof(HANDLE)); //argc is always 5, so 4 will be the number of consumers 

	OpenSpots = CreateSemaphore(NULL, 10, 10, NULL); //counting semaphore
	FilledSpots = CreateSemaphore(NULL, 0, 10, NULL); //counting semaphore
	InWarehouse = CreateSemaphore(NULL, 1, 1, NULL); //binary semaphore
	WritingFiles = CreateSemaphore(NULL, 1, 1, NULL); //binary semaphore
	
	
	pThread = (HANDLE)_beginthreadex(NULL, 0, Producer, argv[1], 0, NULL); //producer: passing the input file
	cThreads = (HANDLE)_beginthreadex(NULL, 0, Consumer, argv[2], CREATE_SUSPENDED, NULL); //consumer: passing the output file

	ResumeThread(cThreads);

	WaitForSingleObject(cThreads, INFINITE);
	WaitForSingleObject(pThread, INFINITE);

	CloseHandle(pThread);
	CloseHandle(cThreads);

	return 0;
}

void WINAPI Producer(char *Filename)
{
	ProducerAlive = TRUE; //default = true
	DWORD BIn;
	char ProducerTemp[2];
	HANDLE ProducerHandle;

	ProducerHandle = CreateFile(Filename,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	ReadFile(ProducerHandle, ProducerTemp, 2, &BIn, NULL); //reads two characters into ProducerTemp

	while (BIn != 0) //not end of file
	{

		WaitForSingleObject(OpenSpots, INFINITE);

		if (BIn == 2) 
		{
			StorageItem[Back].Items[0] = ProducerTemp[0];
			StorageItem[Back].Items[1] = ProducerTemp[1];

			StorageItem[Back].Location = FileLocation; 
		}
		else //if only once character left, the second one will be a space
		{
			StorageItem[Back].Items[0] = ProducerTemp[0];
			StorageItem[Back].Items[1] = ' ';

			StorageItem[Back].Location = FileLocation;
		}

		if (Back < 9) {
			Back++; 
		}

		else {
			Back = 0;
		}

		FileLocation++; 

		WaitForSingleObject(InWarehouse, INFINITE); 

		Count++;

		ReleaseSemaphore(InWarehouse, 1, NULL); 

		ReleaseSemaphore(FilledSpots, 1, NULL); 

		ReadFile(ProducerHandle, ProducerTemp, 2, &BIn, NULL); 

	}

	ProducerAlive = FALSE; //done
}

void WINAPI Consumer(char *Filename)
{
	HANDLE ConsumerHandle;
	DWORD BIn;
	char ConsumerTemp[2]; //does the same thing as ProducesTemp

	ConsumerHandle = CreateFile(Filename,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	while (ProducerAlive == TRUE || Count > 0) 
	{
		WaitForSingleObject(FilledSpots, INFINITE); 
		WaitForSingleObject(InWarehouse, INFINITE); 

		ConsumerTemp[0] = StorageItem[Front].Items[0]; 
		ConsumerTemp[1] = StorageItem[Front].Items[1]; 
												   
		Count--; 

		if (Front < 9) {
			Front++; 
		} 

		else {
			Front = 0;
		}

		ReleaseSemaphore(InWarehouse, 1, NULL);
		ReleaseSemaphore(OpenSpots, 1, NULL); 

		if (EncryptType[0] == 'E') //encrypt or decrypt data
		{
			//encrypt
			Encrypt(StorageItem[Front].Items);
		}
		else if (EncryptType[0] == 'D')
		{
			//decrypt
			Decrypt(StorageItem[Front].Items);
		}

		WaitForSingleObject(WritingFiles, INFINITE); 
		SetFilePointer(ConsumerHandle, StorageItem[Front].Location * 2, NULL, FILE_BEGIN); 

		WriteFile(ConsumerHandle, ConsumerTemp, 2, &BIn, NULL); 

		ReleaseSemaphore(WritingFiles, 1, NULL); 
	}
}