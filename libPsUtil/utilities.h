#pragma once

#define ARRAY_COUNT(arry) sizeof(arry) / sizeof(arry[0])
#define RadToDeg(X) ((float)(X * (180 / M_PI)))
#define DegToRad(X) ((float)(X * (M_PI / 180)))
#define ANGLE2SHORT(x)  ((int)((x)*65536/360) & 65535)
#define SHORT2ANGLE(x)  ((x)*(360.0/65536))

enum SceNotificationRequestType
{
	NotificationRequest = 0,
	SystemNotification = 1,
	SystemNotificationWithUserId = 2,
	SystemNotificationWithDeviceId = 3,
	SystemNotificationWithDeviceIdRelatedToUser = 4,
	SystemNotificationWithText = 5,
	SystemNotificationWithTextRelatedToUser = 6,
	SystemNotificationWithErrorCode = 7,
	SystemNotificationWithAppId = 8,
	SystemNotificationWithAppName = 9,
	SystemNotificationWithAppInfo = 9,
	SystemNotificationWithAppNameRelatedToUser = 10,
	SystemNotificationWithParams = 11,
	SendSystemNotificationWithUserName = 12,
	SystemNotificationWithUserNameInfo = 13,
	SendAddressingSystemNotification = 14,
	AddressingSystemNotificationWithDeviceId = 15,
	AddressingSystemNotificationWithUserName = 16,
	AddressingSystemNotificationWithUserId = 17,

	UNK_1 = 100,
	TrcCheckNotificationRequest = 101,
	NpDebugNotificationRequest = 102,
	UNK_2 = 102,
};

typedef struct
{
	enum SceNotificationRequestType type;
	int reqId;
	int priority;
	int msgId;
	int targetId;
	int userId;
	int unk1;
	int unk2;
	int appId;
	int errorNum;
	int unk3;
	unsigned char useIconImageUri;
	char message[1024];
	char iconUri[1024];
	char unk[1024];
} SceNotificationRequest;

extern int(*sceKernelDebugOutText)(int dbg_channel, const char* text, ...);
extern int(*sceKernelGetModuleInfo)(SceKernelModule handle, SceDbgModuleInfo* info);
extern int(*sceKernelGetModuleList)(SceDbgModule* pArray, size_t numArray, size_t* pActualNum);
extern int(*sceKernelSendNotificationRequests)(int, SceNotificationRequest* request, int, int);
extern int(*sceUserServiceGetForegroundUser)(int*);

bool LoadImports();
uint64_t ResolveFunction(std::string library, std::string function);
void SetBaseAddress();
void klog(const char* fmt, ...);
void Notify(const char* MessageFMT, ...);
void hexdump(void* ptr, int buflen);

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	if (size_s <= 0) { return "Error"; }
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args ...);
	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

std::string to_lower(std::string str);
std::string to_upper(std::string str);
uint64_t GetTick();