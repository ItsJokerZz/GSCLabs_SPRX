#include "stdafx.h"

int(*sceKernelDebugOutText)(int dbg_channel, const char* text, ...);
int(*sceKernelGetModuleInfo)(SceKernelModule handle, SceDbgModuleInfo* info);
int(*sceKernelGetModuleList)(SceDbgModule* pArray, size_t numArray, size_t* pActualNum);
int(*sceKernelSendNotificationRequests)(int, SceNotificationRequest* request, int, int);
int(*sceUserServiceGetForegroundUser)(int*);

bool LoadImports()
{
	int libKernelHandle = sceKernelLoadStartModule("libkernel.sprx", 0, nullptr, 0, nullptr, nullptr);
	int libSceUserHandle = sceKernelLoadStartModule("libSceUserService.sprx", 0, nullptr, 0, nullptr, nullptr);
	if (libKernelHandle > 0 && libSceUserHandle > 0)
	{
		if (sceKernelDlsym(libKernelHandle, "sceKernelDebugOutText", (void**)&sceKernelDebugOutText) != 0)
		{
			printf("Failed to resolve: sceKernelDebugOutText\n");
			return false;
		}

		if (sceKernelDlsym(libKernelHandle, "sceKernelSendNotificationRequest", (void**)&sceKernelSendNotificationRequests) != 0)
		{
			printf("Failed to resolve: sceKernelSendNotificationRequest\n");
			return false;
		}

		if (sceKernelDlsym(libKernelHandle, "sceKernelGetModuleInfo", (void**)&sceKernelGetModuleInfo) != 0)
		{
			printf("Failed to resolve: sceKernelGetModuleInfo\n");
			return false;
		}

		if (sceKernelDlsym(libKernelHandle, "sceKernelGetModuleList", (void**)&sceKernelGetModuleList) != 0)
		{
			printf("Failed to resolve: sceKernelGetModuleList\n");
			return false;
		}

		if (sceKernelDlsym(libSceUserHandle, "sceUserServiceGetForegroundUser", (void**)&sceUserServiceGetForegroundUser) != 0)
		{
			printf("Failed to resolve: sceUserServiceGetForegroundUser\n");
			return false;
		}
	}

	return true;
}

uint64_t ResolveFunction(std::string library, std::string function)
{
	int libraryHandle = sceKernelLoadStartModule(library.data(), 0, nullptr, 0, nullptr, nullptr);
	if (libraryHandle > 0)
	{
		uint64_t libraryFunctionAddr;
		if (sceKernelDlsym(libraryHandle, function.data(), (void**)&libraryFunctionAddr) == 0)
		{
			return libraryFunctionAddr;
		}
		else
			return -1;
	}
	return -1;
}

void SetBaseAddress()
{
	SceDbgModuleInfo info;
	info.size = sizeof(SceDbgModuleInfo);
	sceKernelGetModuleInfo(0, &info);
	BaseAddress = (uint64_t)info.segmentInfo[0].baseAddr;
}

void klog(const char* fmt, ...)
{
	char Buffer[0x200];

	//Create full string from va list.
	va_list args;
	va_start(args, fmt);
	vsprintf(Buffer, fmt, args);
	va_end(args);

	sceKernelDebugOutText(0, Buffer);
}

void Notify(const char* MessageFMT, ...)
{
	SceNotificationRequest Buffer;

	//Create full string from va list.
	va_list args;
	va_start(args, MessageFMT);
	vsprintf(Buffer.message, MessageFMT, args);
	va_end(args);

	//Populate the notify buffer.
	Buffer.type = SceNotificationRequestType::NotificationRequest; //this one is just a standard one and will print what ever is stored at the buffer.Message.
	Buffer.unk3 = 0;
	Buffer.useIconImageUri = 1; //Bool to use a custom uri.
	Buffer.targetId = -1; //Not sure if name is correct but is always set to -1.
	strcpy(Buffer.iconUri, "https://i.imgur.com/SJPIBGG.png"); //Copy the uri to the buffer.

	//From user land we can call int64_t sceKernelSendNotificationRequest(int64_t unk1, char* Buffer, size_t size, int64_t unk2) which is a libkernel import.
	sceKernelSendNotificationRequests(0, &Buffer, 3120, 0);
}

void hexdump(void* ptr, int buflen) {
	unsigned char* buf = (unsigned char*)ptr;
	int i, j;
	for (i = 0; i < buflen; i += 16) {
		printf("%06x: ", i);
		for (j = 0; j < 16; j++)
			if (i + j < buflen)
				printf("%02x ", buf[i + j]);
			else
				printf("   ");
		printf(" ");
		for (j = 0; j < 16; j++)
			if (i + j < buflen)
				printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
		printf("\n");
	}
}

std::string to_lower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

std::string to_upper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

uint64_t GetTick()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}