#pragma once

#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <libdbg.h>
#include <string>
#include <chrono>
#include <algorithm>
#include <memory>

#include <net.h>
#include <sys/socket.h>

#pragma pack(push,1)

template <typename ...Args>
static void server_printf(std::string fmt, Args... args) {
	int size_s = std::snprintf(nullptr, 0, fmt.c_str(), args...) + 1; // Extra space for '\0'
	if (size_s <= 0) { return; }
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, fmt.c_str(), args...);

	int sock = sceNetSocket("PS4 Output", AF_INET, SOCK_STREAM, SCE_NET_IPPROTO_TCP);

	int optval = 1;
	sceNetSetsockopt(sock, SOL_SOCKET, 0x0800, (void*)&optval, sizeof(int));

	SceNetSockaddrIn sockAddr = { 0 };
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = sceNetHtons(9902);
	sceNetInetPton(SCE_NET_AF_INET, "192.168.1.69", &sockAddr.sin_addr);

	sceNetConnect(sock, (SceNetSockaddr*)&sockAddr, sizeof(SceNetSockaddrIn));
	sceNetSend(sock, buf.get(), 0x400, 0);
	sceNetSocketClose(sock);
}

//#define printf server_printf

typedef struct {
	uint8_t len;
	uint8_t p_rep;
	uint8_t p_lock;
	uint8_t p_seg;
	uint8_t p_66;
	uint8_t p_67;
	uint8_t rex;
	uint8_t rex_w;
	uint8_t rex_r;
	uint8_t rex_x;
	uint8_t rex_b;
	uint8_t opcode;
	uint8_t opcode2;
	uint8_t modrm;
	uint8_t modrm_mod;
	uint8_t modrm_reg;
	uint8_t modrm_rm;
	uint8_t sib;
	uint8_t sib_scale;
	uint8_t sib_index;
	uint8_t sib_base;
	union {
		uint8_t imm8;
		uint16_t imm16;
		uint32_t imm32;
		uint64_t imm64;
	} imm;
	union {
		uint8_t disp8;
		uint16_t disp16;
		uint32_t disp32;
	} disp;
	uint32_t flags;
} hde64s;

#pragma pack(pop)

extern "C" unsigned int hde64_disasm(const void* code, hde64s* hs);

extern int(*sceKernelDebugOutText)(int dbg_channel, const char* text, ...);
extern int(*sceKernelGetModuleInfo)(SceKernelModule handle, SceDbgModuleInfo* info);
extern int(*sceKernelGetModuleList)(SceDbgModule* pArray, size_t numArray, size_t* pActualNum);
extern int(*sceKernelSendNotificationRequests)(int, struct SceNotificationRequest* request, int, int);
extern int(*sceUserServiceGetForegroundUser)(int*);

bool LoadImports();
uint64_t ResolveFunction(std::string library, std::string function);
void SetBaseAddress();
void klog(const char* fmt, ...);
void Notify(const char* MessageFMT, ...);
void hexdump(void* ptr, int buflen);

template<typename ... Args>
std::string string_format(std::string format, Args ... args)
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

static uint64_t resolve_module_base(const char* name) {
	SceDbgModule modules[30];
	size_t moduleCount;
	sceKernelGetModuleList(modules, 30, &moduleCount);
	for (int i = 0; i < moduleCount; i++)
	{
		auto& module = modules[i];
		if (module != 0)
		{
			SceDbgModuleInfo info;
			info.size = sizeof(SceDbgModuleInfo);
			sceKernelGetModuleInfo(module, &info);

			if (strstr(info.name, name)) 
			{
				return (uint64_t)info.segmentInfo[0].baseAddr;
			}
		}
	}

	return 0;
}

static SceDbgModuleInfo* resolve_module(const char* name) {
	SceDbgModule modules[30];
	size_t moduleCount;
	sceKernelGetModuleList(modules, 30, &moduleCount);
	for (int i = 0; i < moduleCount; i++)
	{
		auto& module = modules[i];
		if (module != 0)
		{
			SceDbgModuleInfo info;
			info.size = sizeof(SceDbgModuleInfo);
			sceKernelGetModuleInfo(module, &info);

			if (strstr(info.name, name))
			{
				return (SceDbgModuleInfo*)&info;
			}
		}
	}

	return 0;
}

static int get_size(const char* name)
{
	int size;
	SceDbgModule modules[30];
	size_t moduleCount;
	sceKernelGetModuleList(modules, 30, &moduleCount);
	for (int i = 0; i < moduleCount; i++)
	{
		auto& module = modules[i];
		if (module != 0)
		{
			SceDbgModuleInfo info;
			info.size = sizeof(SceDbgModuleInfo);
			sceKernelGetModuleInfo(module, &info);

			if (strstr(info.name, name))
			{
				for (int i = 0; i < info.numSegments; i++)
				{
					if(info.segmentInfo[i].prot == 0x5)
						size += info.segmentInfo[i].size;
				}
			}
		}
	}
	return size;
}
#define INRANGE(x,a,b)   (x >= a && x <= b)
#define GET_BYTE( x )    (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))
#define GET_BITS( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
static uintptr_t find_pattern(const char* module_name, const char* pattern) {
	const auto get_module_size = [=](const char* module_name) {
		return get_size(module_name);
	};

	const auto module_start = (uintptr_t)resolve_module_base(module_name);
	if (module_start != 0ULL) {
		const auto module_end = module_start + get_module_size(module_name);

		const char* pattern_current = pattern;
		uintptr_t current_match = NULL;

		SceKernelVirtualQueryInfo page_information{};
		for (auto current_page = reinterpret_cast<unsigned char*>(module_start); current_page < reinterpret_cast<unsigned char*>(module_end); current_page = reinterpret_cast<unsigned char*>((uint64_t)page_information.start + ((uint64_t)page_information.end - (uint64_t)page_information.start))) {
			sceKernelVirtualQuery(reinterpret_cast<void*>(current_page), SCE_KERNEL_VQ_FIND_NEXT, reinterpret_cast<SceKernelVirtualQueryInfo*>(&page_information), sizeof(SceKernelVirtualQueryInfo));
			if (page_information.protection == PROT_NONE)
				continue;

			for (auto current_address = reinterpret_cast<unsigned char*>(page_information.start); current_address < reinterpret_cast<unsigned char*>((uint64_t)page_information.start + ((uint64_t)page_information.end - (uint64_t)page_information.start) - 0x8); current_address++) {
				if (*current_address != GET_BYTE(pattern_current) && *pattern_current != '\?') {
					current_match = 0ULL;
					pattern_current = pattern;
					continue;
				}

				if (!current_match)
					current_match = reinterpret_cast<uintptr_t>(current_address);

				pattern_current += 3;
				if (pattern_current[-1] == NULL)
					return current_match;
			}
		}
	}

	return 0ULL;
}

extern uint64_t BaseAddress;
#define ResolveAddress(offset) (BaseAddress + (offset))
#define ResolveModuleAddress(base, offset) ((base) + (offset))
#define ResolveModule(name, offset) ((resolve_module_base(base) + (offset)))
template <typename T>
class symbol
{
public:
    size_t offset;
	size_t base;
    T* get() const
    {
        return reinterpret_cast<T*>(((base == 0) ? ResolveAddress(offset) : ResolveModuleAddress(base, offset)));
    }

	void set(size_t offset, size_t base)
	{
		this->offset = offset;
		this->base = base;
	}

	size_t get_offset() { return offset; }

    operator T* () const
    {
        return this->get();
    }

    T* operator->() const
    {
        return this->get();
    }
};
class trampoline
{
protected:
	std::uint8_t* trampoline_bytes = nullptr;
	std::uint8_t* original_bytes = nullptr;
	void* address = nullptr;
	bool set = false;

public:
	virtual void create(void* address, void* destination, uint64_t base) = 0;
	virtual void restore();

	virtual ~trampoline()
	{
		this->restore();
	}
};

class jump : public trampoline
{
public:
	template<class T>
	jump(void* address, T(*function), uint64_t base)
	{
		this->create(address, reinterpret_cast<void*>(function), base);
	}

	template<class T>
	jump(std::uintptr_t address, T(*function)(), uint64_t base)
	{
		this->create(reinterpret_cast<void*>(address), reinterpret_cast<void*>(function), base);
	}

	virtual ~jump()
	{
		this->restore();
	}

	virtual void create(void* address, void* destination, uint64_t base) override;
};

class call : public trampoline
{
private:
	void* original_ptr = nullptr;

public:
	template<class T>
	call(void* address, T(*function), uint64_t base = 0)
	{
		this->create(address, reinterpret_cast<void*>(function), base);
	}

	template<class T>
	call(std::uintptr_t address, T(*function), uint64_t base = 0)
	{
		this->create(reinterpret_cast<void*>(address), reinterpret_cast<void*>(function), base);
	}

	template <typename result, typename... Args>
	result invoke(Args... args)
	{
		result(*f)(Args... args) = decltype(f)(this->original_ptr);
		return f(args...);
	}

	virtual ~call()
	{
		this->restore();
	}

	virtual void create(void* address, void* destination, uint64_t base) override;
};

class detour : public trampoline
{
private:
	void* stub_ptr = nullptr;
	std::size_t stub_size = 0;
	std::size_t original_size = 0u;

public:
	template<class T>
	detour(void* address, T(*function), uint64_t base = 0)
	{
		this->create(address, reinterpret_cast<void*>(function), base);
	}

	template<class T>
	detour(std::uintptr_t address, T(*function), uint64_t base = 0)
	{
		this->create(reinterpret_cast<void*>(address), reinterpret_cast<void*>(function), base);
	}

	template <typename result = void, typename... Args>
	result invoke(Args... args)
	{
		result(*f)(Args... args) = decltype(f)(this->stub_ptr);
		return f(args...);
	}

	virtual ~detour()
	{
		this->restore();
	}

	virtual void create(void* address, void* destination, uint64_t base) override;
	virtual void restore() override;
};

class vec2_t
{
public:
	float x, y;
	vec2_t();
	vec2_t(float x, float y);
	vec2_t(float vec[2]);
	vec2_t operator+(const vec2_t vec);
	vec2_t operator+(const float* vec);
	vec2_t operator-(const vec2_t vec);
	vec2_t operator-(const float* vec);
	vec2_t operator-() const;
	vec2_t operator*(const vec2_t vec);
	vec2_t operator*(const float scalar);
	void operator-=(const vec2_t vec);
	void operator+=(const vec2_t vec);
	void operator*=(const vec2_t vec);
	void operator*=(const float scalar);
	bool operator==(const vec2_t vec);
	bool operator==(const float value);
	bool operator!=(const vec2_t vec);
	bool operator!=(const float value);
	operator float* ();
	vec2_t normalize();
};

class vec3_t
{
public:
	float x, y, z;
	vec3_t();
	vec3_t(float x, float y, float z);
	vec3_t(float vec[3]);
	vec3_t operator+(const vec3_t vec);
	vec3_t operator+(const float* vec);
	vec3_t operator-(const vec3_t vec);
	vec3_t operator-(const float* vec);
	vec3_t operator-() const;
	vec3_t operator*(const vec3_t vec);
	vec3_t operator*(const float scalar);
	void operator-=(const vec3_t vec);
	void operator+=(const vec3_t vec);
	void operator*=(const vec3_t vec);
	void operator*=(const float scalar);
	bool operator==(const vec3_t vec);
	bool operator==(const float value);
	bool operator!=(const vec3_t vec);
	bool operator!=(const float value);
	operator float* ();
	vec3_t forward(float x = 1.0f);
	vec3_t normalize();
	float distance(vec3_t vec);
	float length();
	void vectors(vec3_t* forward, vec3_t* right, vec3_t* up);
	float dot(const vec3_t& vector) { return x * vector.x + y * vector.y + z * vector.z; }
};

class vec4_t
{
public:
	float x, y, z, w;
	vec4_t();
	vec4_t(float x, float y, float z, float w);
	vec4_t(float vec[4]);
	vec4_t operator+(const vec4_t vec);
	vec4_t operator+(const float* vec);
	vec4_t operator-(const vec4_t vec);
	vec4_t operator-(const float* vec);
	vec4_t operator-() const;
	vec4_t operator*(const vec4_t vec);
	vec4_t operator*(const float scalar);
	void operator-=(const vec4_t vec);
	void operator+=(const vec4_t vec);
	void operator*=(const vec4_t vec);
	void operator*=(const float scalar);
	bool operator==(const vec4_t vec);
	bool operator==(const float value);
	bool operator!=(const vec4_t vec);
	bool operator!=(const float value);
	operator float* ();
};