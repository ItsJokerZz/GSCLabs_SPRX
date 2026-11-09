#include "stdafx.h"
#include <libPsUtil.h>

uint64_t BaseAddress;

enum game
{
	NONE,
	IW6MP, // 1.21
	IW6SP, // 1.21
	S1MP, // 1.24
	S1SP, // 1.24
	H1MP, // 1.15
	H1SP, // 1.15
	H2, //1.00
	S2MP, //1.25
	S4MP, //1.26
};

game id = NONE;

bool exists(const std::string& path)
{
	SceKernelStat stat;
	auto res = sceKernelStat(path.c_str(), &stat);
	return res >= 0;
}
struct ScriptFile
{
	const char* name;
	int compressedLen;
	int len;
	int bytecodeLen;
	char* buffer;
	unsigned char* bytecode;
};
void WriteBytes(uint64_t address, const uint8_t* data, size_t size)
{
	memcpy((void*)address, data, size);
}
static const char* ScrStringToCString(const char* s)
{
	if (!s) return nullptr;
	uint16_t len = *(const uint16_t*)s;
	unsigned char c = (unsigned char)s[2];
	if (len < 0x4000 && c >= 0x20 && c <= 0x7E) {
		return s + 2;
	}
	return s;
}

#define DbgPrint printf 
struct RawFile
{
	char* Name;
	int len;
	char* Buffer;
};

void DumpRawFile(const std::string& name, RawFile* rawFile)
{
	if (!rawFile || !rawFile->Buffer || rawFile->len <= 0)
		return;

	std::string baseDir = "/data/rawfiles/";
	std::string filePath = baseDir + name;

	std::replace(filePath.begin(), filePath.end(), '\\', '/');

	FILE* f = fopen(filePath.c_str(), "wb");
	if (f)
	{
		size_t written = fwrite(rawFile->Buffer, 1, rawFile->len, f);
		fclose(f);

		if (written == rawFile->len) {
			DbgPrint("Dumped RawFile: %s (%d bytes) -> %s\n", name.c_str(), rawFile->len, filePath.c_str());
		}
		else {
			DbgPrint("Error: Incomplete write for %s\n", name.c_str());
		}
	}
	else
	{
		DbgPrint("Failed to dump RawFile: %s (Error: %d)\n", name.c_str(), errno);
	}
}


void TestGSCFunction()
{
	Notify("MW2CR Custom GSC Function\nLazy Dev Goated!");
}

namespace GHOMP
{
	detour* DB_FindXAssetHeader_d;
	void* DB_FindXAssetHeader(int type, const char* name, int a)
	{
		void* ret = DB_FindXAssetHeader_d->invoke<void*>(type, name, a);
	
		if (__builtin_return_address(0) == (void*)(0x7A8094))
		{
			if (!strcmp(name, "1499"))
			{
				std::filesystem script("/data/gsc/gho/mp/script.gscbin", "rb");
				
				if(!exists("/data/gsc/gho/mp/script.gscbin"))
					return ret;

				size_t size = script.size();
				script.seek(0, SEEK_SET);
	
				ScriptFile* custom_script = (ScriptFile*)PMem_AllocFromSource_NoDebug(sizeof(ScriptFile) + 1, 4, 2, 5);
				char* allocMemAddress = (char*)PMem_AllocFromSource_NoDebug(size + 1, 4, 2, 5);

				if (allocMemAddress == NULL)
				{
					return ret;
				}
	
				script.read(allocMemAddress, size);
				script.seek(0, SEEK_SET);
	
				while (true)
				{
					char val;
					script.read(&val, 1, 1);
					if (val == '\0')
						break;
				}
	
				int vars[3] = { 0 };
				script.read((char*)vars, sizeof(int) * 3);
				custom_script->compressedLen = vars[0];
				custom_script->len = vars[1];
				custom_script->bytecodeLen = vars[2];
				custom_script->buffer = allocMemAddress + (int)script.tell();
				custom_script->bytecode = (unsigned char*)(allocMemAddress + vars[0] + (int)script.tell());
				return (void*)custom_script;
			}
		}
		return ret;
	}

	//static symbol <int()>Com_GetCurrentCoDPlayMode{ 0x885100 };
	struct scr_entref_t
	{
		unsigned short entnum;
		unsigned short classnum;
	};
	enum he_type_t : int
	{
		HE_TYPE_FREE,
		HE_TYPE_TEXT,
		HE_TYPE_VALUE,
		HE_TYPE_PLAYERNAME,
		HE_TYPE_MATERIAL,
		HE_TYPE_TIMER_DOWN,
		HE_TYPE_TIMER_UP,
		HE_TYPE_TIMER_STATIC,
		HE_TYPE_TENTHS_TIMER_DOWN,
		HE_TYPE_TENTHS_TIMER_UP,
		HE_TYPE_TENTHS_TIMER_STATIC,
		HE_TYPE_CLOCK_DOWN,
		HE_TYPE_CLOCK_UP,
		HE_TYPE_WAYPOINT,
		HE_TYPE_COUNT
	};
	struct hudelem_s
	{
	public:
		he_type_t type; //0x0
	private:
		char pad[0x80]; //0x4
	public:
		int text; //0x84
	private:
		char _pad[0x30]; //0x88
	}; //Size: 0xB8
	struct game_hudelem_s
	{
		hudelem_s elem; //0x0
	}; //Size: 0xB8
	#define g_hudelems (*(game_hudelem_s**)0x38FE790)
	game_hudelem_s* HECmd_GetHudElem(scr_entref_t entref)
	{
		if (entref.classnum == 1)
			return &g_hudelems[entref.entnum];

		return NULL;
	}

	void HECmd_ClearAllTextAfterHudElem(scr_entref_t entref)
	{
		game_hudelem_s* hud = HECmd_GetHudElem(entref);
		if (hud && hud->elem.type == HE_TYPE_TEXT)
		{
			if (hud->elem.text + 1 < 511)
			{
				for (int i = hud->elem.text + 541; i < 1051; ++i)
				{
					SV_SetConfigstring(i, "");
				}
			}
		}
	}
}
namespace GHOSP
{
	detour* DB_FindXAssetHeader_d;
	void* DB_FindXAssetHeader(int type, const char* name, int a)
	{
		void* ret = DB_FindXAssetHeader_d->invoke<void*>(type, name, a);

		if (__builtin_return_address(0) == (void*)(0x73E504))
		{
			if (strcmp(name, "988") == 0)
			{
				std::filesystem script("/data/gsc/gho/sp/script.gscbin", "rb");

				if (!exists("/data/gsc/gho/sp/script.gscbin"))
					return ret;

				int size = (int)script.size();
				script.seek(0, SEEK_SET);

				auto PMem_AllocFromSource_NoDebug = reinterpret_cast<void* (*)(uint64_t, uint64_t, int, int)>(0x79E8C0);
				ScriptFile* custom_script = (ScriptFile*)PMem_AllocFromSource_NoDebug(sizeof(ScriptFile) + 1, 4, 2, 5);

				char* allocMemAddress = (char*)PMem_AllocFromSource_NoDebug(size + 1, 4, 2, 5);


				script.read(allocMemAddress, size);
				script.seek(0, SEEK_SET);

				while (true)
				{
					char val;
					script.read(&val, 1, 1);
					if (val == '\0')
						break;
				}

				int vars[3] = { 0 };
				script.read((char*)vars, sizeof(int) * 3);

				custom_script->compressedLen = vars[0];
				custom_script->len = vars[1];
				custom_script->bytecodeLen = vars[2];
				custom_script->buffer = allocMemAddress + (int)script.tell();
				custom_script->bytecode = (unsigned char*)(allocMemAddress + vars[0] + (int)script.tell());

				return (void*)custom_script;
			}
		}
		return ret;
	}
}
namespace AWMP
{

	enum he_type_t
	{
		HE_TYPE_FREE,
		HE_TYPE_TEXT,
		HE_TYPE_VALUE,
		HE_TYPE_PLAYERNAME,
		HE_TYPE_MATERIAL,
		HE_TYPE_TIMER_DOWN,
		HE_TYPE_TIMER_UP,
		HE_TYPE_TIMER_STATIC,
		HE_TYPE_TENTHS_TIMER_DOWN,
		HE_TYPE_TENTHS_TIMER_UP,
		HE_TYPE_TENTHS_TIMER_STATIC,
		HE_TYPE_CLOCK_DOWN,
		HE_TYPE_CLOCK_UP,
		HE_TYPE_WAYPOINT,
		HE_TYPE_COUNT
	};

	struct hudelem_s
	{
	private:
		char pad[0x1C]; //0x0
	public:
		he_type_t type; //0x1C
	private:
		char _pad[0x64]; //0x20
	public:
		int text; //0x84
	private:
		char __pad[0x34]; //0x88
	}; //Size: 0xBC

	struct game_hudelem_s
	{
		hudelem_s elem; //0x0
	}; //Size: 0xBC
	#define g_hudelems (*(game_hudelem_s**)0xD7F990)

	struct scr_entref_t
	{
		unsigned short entnum;
		unsigned short classnum;
	};

	game_hudelem_s* HECmd_GetHudElem(scr_entref_t entref)
	{
		if (entref.classnum == 1)
			return &g_hudelems[entref.entnum];

		return NULL;
	}

	void HECmd_ClearAllTextAfterHudElem(scr_entref_t entref)
	{
		game_hudelem_s* hud = HECmd_GetHudElem(entref);
		if (hud && hud->elem.type == HE_TYPE_TEXT)
		{
			if (hud->elem.text + 1 < 650)
			{
				for (int i = hud->elem.text + 541; i < 1190; ++i)
				{
					SV_SetConfigstring(i, "");
				}
			}
		}
	}

	detour* DB_FindXAssetHeader_t;
	void* DB_FindXAssetHeader(int type, const char* name, int a)
	{
		void* ret = DB_FindXAssetHeader_t->invoke<void*>(type, name, a);

		if (__builtin_return_address(0) == (void*)(0x855594))
		{
			if (strcmp(name, "42918") == 0)
			{
				std::filesystem script("/data/gsc/aw/mp/script.gscbin", "rb");

				if (!exists("/data/gsc/aw/mp/script.gscbin"))
					return ret;

				int size = (int)script.size();
				script.seek(0, SEEK_SET);

				ScriptFile* custom_script = (ScriptFile*)PMem_AllocFromSource_NoDebug(sizeof(ScriptFile) + 1, 4, 2, 5);

				char* allocMemAddress = (char*)PMem_AllocFromSource_NoDebug(size + 1, 4, 2, 5);


				script.read(allocMemAddress, size);
				script.seek(0, SEEK_SET);

				while (true)
				{
					char val;
					script.read(&val, 1, 1);
					if (val == '\0')
						break;
				}

				int vars[3] = { 0 };
				script.read((char*)vars, sizeof(int) * 3);

				custom_script->compressedLen = vars[0];
				custom_script->len = vars[1];
				custom_script->bytecodeLen = vars[2];
				custom_script->buffer = allocMemAddress + (int)script.tell();
				custom_script->bytecode = (unsigned char*)(allocMemAddress + vars[0] + (int)script.tell());

				return (void*)custom_script;
			}
		}
		return ret;
	}
}
namespace AWSP
{
	detour* DB_FindXAssetHeader_d;
	void* DB_FindXAssetHeader(int type, const char* name, int a)
	{
		void* ret = DB_FindXAssetHeader_d->invoke<void*>(type, name, a);

		if (__builtin_return_address(0) == (void*)(0x75AAE4))
		{
			if (strcmp(name, "42299") == 0)
			{
				std::filesystem script("/data/gsc/aw/sp/script.gscbin", "rb");

				if (!exists("/data/gsc/aw/sp/script.gscbin"))
					return ret;

				int size = (int)script.size();
				script.seek(0, SEEK_SET);

				auto PMem_AllocFromSource_NoDebug = reinterpret_cast<void* (*)(uint64_t, uint64_t, int, int)>(0x7B6A40);
				ScriptFile* custom_script = (ScriptFile*)PMem_AllocFromSource_NoDebug(sizeof(ScriptFile) + 1, 4, 2, 5);

				char* allocMemAddress = (char*)PMem_AllocFromSource_NoDebug(size + 1, 4, 2, 5);


				script.read(allocMemAddress, size);
				script.seek(0, SEEK_SET);

				while (true)
				{
					char val;
					script.read(&val, 1, 1);
					if (val == '\0')
						break;
				}

				int vars[3] = { 0 };
				script.read((char*)vars, sizeof(int) * 3);

				custom_script->compressedLen = vars[0];
				custom_script->len = vars[1];
				custom_script->bytecodeLen = vars[2];
				custom_script->buffer = allocMemAddress + (int)script.tell();
				custom_script->bytecode = (unsigned char*)(allocMemAddress + vars[0] + (int)script.tell());

				return (void*)custom_script;
			}
		}
		return ret;
	}
}
namespace MWRMP
{
	typedef unsigned char byte;
	#define Symbol(Name, Offset) static Name##_t Name = Name##_t(Offset)
	typedef void(*SV_SetConfigstring_t)(unsigned int a1, const char* a2);
	Symbol(SV_SetConfigstring, 0xC4D8F0);

	detour* DB_FindXAssetHeader_d;
	void* DB_FindXAssetHeader(int type, const char* name, int a)
	{
		void* ret = DB_FindXAssetHeader_d->invoke<void*>(type, name, a);

		if (type == 48)
		{
			if (!strcmp(name, "42928"))
			{
				std::filesystem script("/data/gsc/mwr/mp/script.gscbin", "rb");

				int size = (int)script.size();
				script.seek(0, SEEK_SET);

				auto PMem_AllocFromSource_NoDebug = reinterpret_cast<void* (*)(uint64_t, uint64_t, int, int)>(0xCC69A0);
				ScriptFile* custom_script = (ScriptFile*)PMem_AllocFromSource_NoDebug(sizeof(ScriptFile) + 1, 4, 2, 5);

				char* allocMemAddress = (char*)PMem_AllocFromSource_NoDebug(size + 1, 4, 2, 5);

				script.read(allocMemAddress, size);
				script.seek(0, SEEK_SET);

				while (true)
				{
					char val;
					script.read(&val, 1, 1);
					if (val == '\0')
						break;
				}

				int vars[3] = { 0 };
				script.read((char*)vars, sizeof(int) * 3);

				custom_script->compressedLen = vars[0];
				custom_script->len = vars[1];
				custom_script->bytecodeLen = vars[2];
				custom_script->buffer = allocMemAddress + script.tell();
				custom_script->bytecode = (unsigned char*)(allocMemAddress + vars[0] + script.tell());
				return (void*)custom_script;
			}
		}
		return ret;
	}

	struct scr_entref_t
	{
		unsigned short entnum;
		unsigned short classnum;
	};
	enum he_type_t : int
	{
		HE_TYPE_FREE,
		HE_TYPE_TEXT,
		HE_TYPE_VALUE,
		HE_TYPE_PLAYERNAME,
		HE_TYPE_MATERIAL,
		HE_TYPE_TIMER_DOWN,
		HE_TYPE_TIMER_UP,
		HE_TYPE_TIMER_STATIC,
		HE_TYPE_TENTHS_TIMER_DOWN,
		HE_TYPE_TENTHS_TIMER_UP,
		HE_TYPE_TENTHS_TIMER_STATIC,
		HE_TYPE_CLOCK_DOWN,
		HE_TYPE_CLOCK_UP,
		HE_TYPE_WAYPOINT,
		HE_TYPE_COUNT
	};

	union hudelem_color_t
	{
		struct
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		};
		int rgba;
	};
	struct hudelem_s
	{
	public:
		short targetEntNum; //0x0
		short triggerEntNum; //0x2
		int font; //0x4
		int alignOrg; //0x8
		int alignScreen; //0xC
		float x; //0x10
		float y; //0x14
		float z; //0x18
		he_type_t type; //0x1C
		float fontScale; //0x20
		float fromFontScale; //0x24
		int fontScaleStartTime; //0x28
		int fontScaleTime; //0x2C
		hudelem_color_t color; //0x30
		hudelem_color_t fromColor; //0x34
		int fadeStartTime; //0x38
		int fadeTime; //0x3C
		int label; //0x40
		int width; //0x44
		int height; //0x48
		int materialIndex; //0x4C
		int fromWidth; //0x50
		int fromHeight; //0x54
		int scaleStartTime; //0x58
		int scaleTime; //0x5C
		float fromX; //0x60
		float fromY; //0x64
		int fromAlignOrg; //0x68
		int fromAlignScreen; //0x6C
		int moveStartTime; //0x70
		int moveTime; //0x74
		int time; //0x78
		int duration; //0x7C
		float value; //0x80
		int text; //0x84
		float sort; //0x88
		hudelem_color_t glowColor; //0x8C
		int fxBirthTime; //0x90
		int fxLetterTime; //0x94
		int fxDecayStartTime; //0x98
		int fxDecayDuration; //0x9C
		int soundID; //0xA0
		int boneIndex; //0xA4
		int flags; //0xA8
	private:
		int _pad_AC;
		long _pad_B0;
		short _pad_B8;
		short _pad_BA;
		int _pad_BC;
	};
	struct game_hudelem_s
	{
		hudelem_s elem; //0x0
		int clientNum; //0xC0
		int team; //0xC4
		int archived; //0xC8
		int currentShowInKillcam; //0xCC
	};
	#define g_hudelems ((game_hudelem_s*)0xB0BC840)

	game_hudelem_s* HECmd_GetHudElem(scr_entref_t entref)
	{
		if (entref.classnum == 1)
		{
			return &g_hudelems[entref.entnum];
		}
		else
		{
			return NULL;
		}
	}

	void HECmd_ClearAllTextAfterHudElem(scr_entref_t entref)
	{
		game_hudelem_s* hud = HECmd_GetHudElem(entref);
		if (hud && hud->elem.type == HE_TYPE_TEXT)
		{
			if (hud->elem.text && hud->elem.text + 1 < 650)
			{
				for (int i = hud->elem.text + 541; i < 1190; ++i)
				{
					SV_SetConfigstring(i, "");
				}
			}
		}
	}
}
namespace MWRSP
{
	detour* DB_FindXAssetHeader_d;
	void* DB_FindXAssetHeader(int type, const char* name, int a)
	{
		void* ret = DB_FindXAssetHeader_d->invoke<void*>(type, name, a);

		if (type == 0x30)
		{
			if (!strcmp(name, "42309"))
			{
				std::filesystem script("/data/gsc/mwr/sp/script.gscbin", "rb");

				if (!exists("/data/gsc/mwr/sp/script.gscbin"))
					return ret;

				int size = (int)script.size();
				script.seek(0, SEEK_SET);

				auto PMem_AllocFromSource_NoDebug = reinterpret_cast<void* (*)(uint64_t, uint64_t, int, int)>(0xA40000);
				ScriptFile* custom_script = (ScriptFile*)PMem_AllocFromSource_NoDebug(sizeof(ScriptFile) + 1, 4, 2, 5);

				char* allocMemAddress = (char*)PMem_AllocFromSource_NoDebug(size + 1, 4, 2, 5);

				script.read(allocMemAddress, size);
				script.seek(0, SEEK_SET);

				while (true)
				{
					char val;
					script.read(&val, 1, 1);
					if (val == '\0')
						break;
				}

				int vars[3] = { 0 };
				script.read((char*)vars, sizeof(int) * 3);

				custom_script->compressedLen = vars[0];
				custom_script->len = vars[1];
				custom_script->bytecodeLen = vars[2];
				custom_script->buffer = allocMemAddress + (int)script.tell();
				custom_script->bytecode = (unsigned char*)(allocMemAddress + vars[0] + (int)script.tell());
				return (void*)custom_script;
			}
		}
		return ret;
	}
}
namespace MW2CR
{
	uintptr_t scrVarPub_stringPool = 0xC3C4178;
	uintptr_t scr_OutParamCount = 0xCD09230;
	uintptr_t* scr_StackPtr = (uintptr_t*)0xCD09228;

	typedef uint32_t(*SL_GetString_t)(const char* str, unsigned int user);
	SL_GetString_t SL_GetString = (SL_GetString_t)0x9FC300;

	typedef void(*Scr_AddFloat_t)(float value);
	Scr_AddFloat_t Scr_AddFloat = (Scr_AddFloat_t)0x9FB920;

	int Scr_GetNumParam()
	{
		return *(int*)(scr_OutParamCount + 0x4);
	}
	const char* Scr_GetString(unsigned int paramIndex)
	{
		uintptr_t stackTop = *(uintptr_t*)0xCD09228;

		unsigned int stringHandle = *(unsigned int*)(stackTop - (paramIndex * 8));

		if (stringHandle == 0) return nullptr;

		uintptr_t poolBase = *(uintptr_t*)scrVarPub_stringPool;
		const char* str = (const char*)(poolBase + (16 * stringHandle) + 8);

		return str;
	}
	/*void Scr_AddInt(int value)
	{
		uintptr_t currentStack = *scr_StackPtr;

		uintptr_t newEntry = currentStack + 16;

		*(int*)(newEntry + 8) = 6;

		*(int*)(newEntry + 0) = value;

		*scr_StackPtr = newEntry;
		(*scr_OutParamCount)++;
	}
	int Scr_GetInt(unsigned int index)
	{
		uintptr_t stackTop = *scr_StackPtr;
		uintptr_t paramBase = stackTop - (index * 16);

		int type = *(int*)(paramBase - 8);

		if (type == 6 || type == 7)
		{
			return *(int*)(paramBase - 16);
		}
		return 0;
	}
	void Scr_AddString(const char* value)
	{
		if (!value)
			value = "";
		uint32_t stringIndex = SL_GetString(value, 0);
		uintptr_t currentStack = *scr_StackPtr;
		uintptr_t newEntry = currentStack + 16;
		*(uint32_t*)(newEntry + 0) = stringIndex;
		*(int*)(newEntry + 8) = 4;
		*scr_StackPtr = newEntry;
		(*scr_OutParamCount)++;
	}*/

	void GScr_xNotify()
	{
		if (!Scr_GetNumParam() || !Scr_GetString(0))
			return;
		if (Scr_GetNumParam() < 1)
			return;

		const char* text = Scr_GetString(0);
		if (!text) return;

		Notify(text);
	}
	void GScr_WriteBytes()
	{
		int arg = Scr_GetNumParam();
		if (arg < 2) return;
		const char* rawAddStr = Scr_GetString(0);
		if (!rawAddStr) return;
		const char* cleanAddr = rawAddStr;
		while (*cleanAddr && !isxdigit((unsigned char)*cleanAddr)) {
			cleanAddr++;
		}
		uintptr_t address = (uintptr_t)strtoull(cleanAddr, nullptr, 16);
		std::vector<uint8_t> buf;
		for (int i = 1; i < 16; ++i)
		{
			const char* bstr = Scr_GetString(i);
			if (bstr)
			{
				const char* cleanByte = bstr;
				while (*cleanByte && !isxdigit((unsigned char)*cleanByte)) {
					cleanByte++;
				}

				if (*cleanByte) {
					uint8_t val = (uint8_t)strtoul(cleanByte, nullptr, 16);
					buf.push_back(val);
				}
			}
		}
		if (!buf.empty())
		{
			size_t writeSize = buf.size();

			uintptr_t pageBase = address & ~0xFFF;
			size_t pageSize = ((address + writeSize) - pageBase + 0xFFF) & ~0xFFF;

			if (sceKernelMprotect((void*)pageBase, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) == 0)
			{
				memcpy((void*)address, buf.data(), buf.size());

				sceKernelMprotect((void*)pageBase, pageSize, PROT_READ | PROT_EXEC);
			}
		}
	}
	void GScr_WriteInts()
	{
		int arg = Scr_GetNumParam();
		if (arg < 2) return;

		const char* addStr = Scr_GetString(0);
		if (!addStr) return;

		const char* cleanAddr = addStr;
		while (*cleanAddr && !isxdigit((unsigned char)*cleanAddr)) cleanAddr++;
		uintptr_t address = (uintptr_t)strtoull(cleanAddr, nullptr, 16);

		std::vector<int32_t> buf;
		for (int i = 1; i < arg; ++i)
		{
			const char* vstr = Scr_GetString(i);
			if (vstr)
			{
				const char* cleanVal = vstr;
				while (*cleanVal && !isxdigit((unsigned char)*cleanVal) && *cleanVal != '-') cleanVal++;

				int32_t val = (int32_t)strtol(cleanVal, nullptr, 0);
				buf.push_back(val);
			}
		}

		if (!buf.empty())
		{
			size_t totalSize = buf.size() * sizeof(int32_t);

			uintptr_t pageBase = address & ~0xFFF;
			size_t pageSize = ((address + totalSize) - pageBase + 0xFFF) & ~0xFFF;

			if (sceKernelMprotect((void*)pageBase, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) == 0)
			{
				memcpy((void*)address, buf.data(), totalSize);

				sceKernelMprotect((void*)pageBase, pageSize, PROT_READ | PROT_EXEC);
			}
		}
	}
	void GScr_WriteFloats()
	{
		int arg = Scr_GetNumParam();
		if (arg < 2) return;

		const char* rawAddStr = Scr_GetString(0);
		if (!rawAddStr) return;

		const char* cleanAddr = rawAddStr;
		while (*cleanAddr && !isxdigit((unsigned char)*cleanAddr)) {
			cleanAddr++;
		}
		uintptr_t address = (uintptr_t)strtoull(cleanAddr, nullptr, 16);

		std::vector<float> buf;
		for (int i = 1; i < arg; ++i)
		{
			const char* fstr = Scr_GetString(i);
			if (fstr)
			{
				const char* cleanVal = fstr;
				while (*cleanVal && !isdigit((unsigned char)*cleanVal) && *cleanVal != '.' && *cleanVal != '-') {
					cleanVal++;
				}

				if (*cleanVal) {
					buf.push_back(strtof(cleanVal, nullptr));
				}
			}
		}

		if (!buf.empty())
		{
			size_t writeSize = buf.size() * sizeof(float);
			uintptr_t pageBase = address & ~0xFFF;
			size_t pageSize = ((address + writeSize) - pageBase + 0xFFF) & ~0xFFF;

			if (sceKernelMprotect((void*)pageBase, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) == 0)
			{
				float* dest = (float*)address;
				for (size_t j = 0; j < buf.size(); ++j) {
					dest[j] = buf[j];
				}

				sceKernelMprotect((void*)pageBase, pageSize, PROT_READ | PROT_EXEC);
			}
		}
	}
	void GScr_WriteString()
	{
		int arg = Scr_GetNumParam();
		if (arg < 2) return;

		const char* addStrRaw = Scr_GetString(0);
		const char* addStr = ScrStringToCString(addStrRaw);
		if (!addStr) return;

		const char* p = strstr(addStr, "0x");
		if (!p) p = strstr(addStr, "0X");
		if (p) p += 2;
		else {
			p = addStr;
			while (*p && !isxdigit((unsigned char)*p)) p++;
		}

		char* end = nullptr;
		uintptr_t address = (uintptr_t)strtoull(p, &end, 16);
		if (end == p || !address) return;

		const char* textRaw = Scr_GetString(1);
		const char* textToWrite = ScrStringToCString(textRaw);
		if (!textToWrite) return;

		size_t totalSize = strlen(textToWrite) + 1;

		uintptr_t pageBase = address & ~0xFFF;
		size_t pageSize = ((address + totalSize) - pageBase + 0xFFF) & ~0xFFF;

		if (sceKernelMprotect((void*)pageBase, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) == 0)
		{
			memcpy((void*)address, textToWrite, totalSize);
			sceKernelMprotect((void*)pageBase, pageSize, PROT_READ | PROT_EXEC);
		}
	}

	detour* DB_FindXAssetHeader_d;
	ScriptFile* DB_FindXAssetHeader(int type, const char* name, int a)
	{
		ScriptFile* ret = DB_FindXAssetHeader_d->invoke<ScriptFile*>(type, name, a);

		if (type == 48)
		{
			if (strcmp(name, "50895") == 0)
			{
				std::filesystem script("/data/gsc/mw2cr/script.gscbin", "rb");

				if (!exists("/data/gsc/mw2cr/script.gscbin"))
					return ret;

				int size = (int)script.size();
				script.seek(0, SEEK_SET);

				auto PMem_AllocFromSource_NoDebug = reinterpret_cast<void* (*)(uint64_t, uint64_t, int, int)>(0xA54180);
				ScriptFile* custom_script = (ScriptFile*)PMem_AllocFromSource_NoDebug(sizeof(ScriptFile) + 1, 4, 2, 5);

				char* allocMemAddress = (char*)PMem_AllocFromSource_NoDebug(size + 1, 4, 2, 5);


				script.read(allocMemAddress, size);
				script.seek(0, SEEK_SET);

				while (true)
				{
					char val;
					script.read(&val, 1, 1);
					if (val == '\0')
						break;
				}

				int vars[3] = { 0 };
				script.read((char*)vars, sizeof(int) * 3);

				custom_script->compressedLen = vars[0];
				custom_script->len = vars[1];
				custom_script->bytecodeLen = vars[2];
				custom_script->buffer = allocMemAddress + (int)script.tell();
				custom_script->bytecode = (unsigned char*)(allocMemAddress + vars[0] + (int)script.tell());

				return custom_script;
			}
		}
		return ret;
	}
}
namespace WW2MP
{
	enum XAssetType : int
	{
		ASSET_TYPE_PHYSPRESET = 0,
		ASSET_TYPE_SNDPHYSPRESET,
		ASSET_TYPE_SNDMUSICSET,
		ASSET_TYPE_PHYS_COLLMAP,
		ASSET_TYPE_PHYSWATERPRESET,
		ASSET_TYPE_PHYS_WORLDMAP,
		ASSET_TYPE_PHYSCONSTRAINT,
		ASSET_TYPE_XANIM,
		ASSET_TYPE_XSURFSHARED,
		ASSET_TYPE_XMODELSURFS,
		ASSET_TYPE_XMODEL,
		ASSET_TYPE_XMODELBASE,
		ASSET_TYPE_MAYHEM,
		ASSET_TYPE_MATERIAL,
		ASSET_TYPE_COMPUTESHADER,
		ASSET_TYPE_VERTEXSHADER,
		ASSET_TYPE_HULLSHADER,
		ASSET_TYPE_DOMAINSHADER,
		ASSET_TYPE_PIXELSHADER,
		ASSET_TYPE_TECHSET,
		ASSET_TYPE_IMAGE,
		ASSET_TYPE_SOUND,
		ASSET_TYPE_SOUNDSUBMIX,
		ASSET_TYPE_SNDCURVE,
		ASSET_TYPE_DISTCURVE,
		ASSET_TYPE_REVERBSENDCURVE,
		ASSET_TYPE_SNDCONTEXT,
		ASSET_TYPE_ALIASPARAMETERMODIFIER,
		ASSET_TYPE_ALIASCOMBATCONE,
		ASSET_TYPE_LOADED_SOUND,
		ASSET_TYPE_COL_MAP_MP,
		ASSET_TYPE_COM_MAP,
		ASSET_TYPE_GLASS_MAP,
		ASSET_TYPE_AIPATHS,
		ASSET_TYPE_NAVMESH,
		ASSET_TYPE_VEHICLE_TRACK,
		ASSET_TYPE_MAP_ENTS,
		ASSET_TYPE_FX_MAP,
		ASSET_TYPE_GFX_MAP,
		ASSET_TYPE_GFX_MAP_TRZONE,
		ASSET_TYPE_COL_MAP_TRZONE,
		ASSET_TYPE_IESPROFILE,
		ASSET_TYPE_LIGHTDEF,
		ASSET_TYPE_UI_MAP,
		ASSET_TYPE_ANIMCLASS,
		ASSET_TYPE_LOCALIZE,
		ASSET_TYPE_ATTACHMENT,
		ASSET_TYPE_WEAPON,
		ASSET_TYPE_SNDDRIVERGLOBALS,
		ASSET_TYPE_FX,
		ASSET_TYPE_IMPACTFX,
		ASSET_TYPE_SURFACEFX,
		ASSET_TYPE_AITYPE,
		ASSET_TYPE_MPTYPE,
		ASSET_TYPE_CHARACTER,
		ASSET_TYPE_XMODELALIAS,
		ASSET_TYPE_RAWFILE,
		ASSET_TYPE_SCRIPTFILE,
		ASSET_TYPE_STRINGTABLE,
		ASSET_TYPE_LEADERBOARDDEF,
		ASSET_TYPE_VIRTUALLEADERBOARDDEF,
		ASSET_TYPE_STRUCTUREDDATADEF,
		ASSET_TYPE_TRACER,
		ASSET_TYPE_VEHICLE,
		ASSET_TYPE_ADDON_MAP_ENTS,
		ASSET_TYPE_NETCONSTSTRINGS,
		ASSET_TYPE_REVERBPRESET,
		ASSET_TYPE_LUAFILE,
		ASSET_TYPE_SCRIPTABLE,
		ASSET_TYPE_EQUIPSNDTABLE,
		ASSET_TYPE_VECTORFIELD,
		ASSET_TYPE_PARTICLESIMANIMATION,
		ASSET_TYPE_LASER,
		ASSET_TYPE_BEAM,
		ASSET_TYPE_SKELETONSCRIPT,
		ASSET_TYPE_CLUT,
		ASSET_TYPE_SPLINES,
		ASSET_TYPE_PHYSCLOTHTUNING,
		ASSET_TYPE_DLOGSCHEMA,
		ASSET_TYPE_DLOGROUTES,

		ASSET_TYPE_COUNT
	};
}

struct GameDetectionEntry
{
	const char* address;
	game id;
};

std::vector<GameDetectionEntry> gameDetectionTable = {
	{reinterpret_cast<const char*>(0xA949BB), S1SP},
	{reinterpret_cast<const char*>(0xB06FDA), IW6SP},
	{reinterpret_cast<const char*>(0xC0B0E7), S1MP},
	{reinterpret_cast<const char*>(0xC251D7), IW6MP},
	{reinterpret_cast<const char*>(0xD46DEE), H2},
	{reinterpret_cast<const char*>(0xD5E106), H1SP},
	{reinterpret_cast<const char*>(0xF991A9), H1MP},
	{reinterpret_cast<const char*>(0x13D7939), S2MP},
};

void game_detection(game& id)
{
	id = NONE;
	for (const auto& entry : gameDetectionTable)
	{
		if (!strcmp(entry.address, "IsMultiplayer"))
		{
			id = entry.id;
			return;
		}
	}
}
extern "C" int module_start(size_t args, const void* argp)
{
	ScePthread entry_thread;
	scePthreadCreate(&entry_thread, nullptr, [](void*) -> void* {

		LoadImports();
		SetBaseAddress();

		game_detection(id);

		Notify("GSC Loader\nby: Skidful");

		switch (int(id))
		{
			case IW6MP:
			{
				ResolveFunctions();
				sceKernelMprotect((const void*)(0x42AB1C), 1, 7);
				*(char*)(0x42AB1C) = 0x75;

				sceKernelMprotect((const void*)(0x38E5750), 8, 7);
				*(unsigned long*)(0x38E5750) = (unsigned long)GHOMP::HECmd_ClearAllTextAfterHudElem;

				//sceKernelMprotect((const void*)0x38E9530, 8, 7);
				//*(unsigned long*)(0x38E9530) = (unsigned long)TestGSCFunction;

				GHOMP::DB_FindXAssetHeader_d = new detour(0x65FC10, GHOMP::DB_FindXAssetHeader);

				Notify("Ghosts Mulitplayer\nGSC Loader Loaded!!");
			}
			break;
			case IW6SP:
			{
				GHOSP::DB_FindXAssetHeader_d = new detour(0x5A1250, GHOSP::DB_FindXAssetHeader);
				Notify("Ghosts Singleplayer\nGSC Loader Loaded");
			}
			break;
			case S1MP:
			{
				AWMPFunctions();
				//Hooks DB_FindXAssetHeader
				AWMP::DB_FindXAssetHeader_t = new detour(0x6A4570, AWMP::DB_FindXAssetHeader);
				sceKernelMprotect((const void*)0xD60B20, 8, 7);
				*(unsigned long*)(0xD60B20) = (unsigned long)AWMP::HECmd_ClearAllTextAfterHudElem;
				//sceKernelMprotect((const char*)0xD63AD0, 8, 7);
				//*(unsigned long*)(0xD63AD0) = (unsigned long)TestFunctionGSC;

				Notify("AW Multiplayer\nGSC Loader Loaded");
			}
			break;
			case S1SP:
			{
				AWSP::DB_FindXAssetHeader_d = new detour(0x5981F0, AWSP::DB_FindXAssetHeader);
				Notify("AW Singleplayer\nGSC Loader Loaded");
			}
			break;
			case H1MP:
			{
				MWRMP::DB_FindXAssetHeader_d = new detour(0x9B6590, MWRMP::DB_FindXAssetHeader);
				sceKernelMprotect((const void*)0x105FDC0, 8, 7);

				//Depatch Bounces
				//sceKernelMprotect((const void*)0x8A324D, 8, 7);
				//WriteBytes(0x8A324D, (uint8_t*)"\x80\x7D\xAB\x01", 4);

				*(unsigned long*)(0x105FDC0) = (unsigned long)MWRMP::HECmd_ClearAllTextAfterHudElem;

				Notify("MWR Multiplayer\nGSC Loader Loaded");
			}
			break;
			case H1SP:
			{
				MWRSP::DB_FindXAssetHeader_d = new detour(0x6F4C30, MWRSP::DB_FindXAssetHeader);
				Notify("MWR Singleplayer\nGSC Loader Loaded");
			}
			break;
			case H2:
			{
				sceKernelMprotect((const void*)0x1015B20, 8, 7);
				*(unsigned long*)(0x1015B20) = (unsigned long)MW2CR::GScr_WriteString; //_func_02BB

				sceKernelMprotect((const void*)0x1015B30, 8, 7);
				*(unsigned long*)(0x1015B30) = (unsigned long)MW2CR::GScr_WriteBytes; //_func_02BC

				sceKernelMprotect((const void*)0x1015B40, 8, 7);
				*(unsigned long*)(0x1015B40) = (unsigned long)MW2CR::GScr_xNotify; //_func_02E9

				sceKernelMprotect((const void*)0x1015B50, 8, 7);
				*(unsigned long*)(0x1015B50) = (unsigned long)MW2CR::GScr_WriteInts; //_func_02EA

				sceKernelMprotect((const void*)0x1015B60, 8, 7);
				*(unsigned long*)(0x1015B60) = (unsigned long)MW2CR::GScr_WriteFloats; //_func_02EB

				MW2CR::DB_FindXAssetHeader_d = new detour(0x736480, MW2CR::DB_FindXAssetHeader);

				Notify("MW2CR\nGSC Loader Loaded");
			}
			break;
			case S2MP:
			{
				//WW2MPFunctions();
				//WW2MP::DB_FindXAssetHeader_t = new detour(0x723440, WW2MP::DB_FindXAssetHeader);
				Notify("WW2 Multiplayer\nGSC Work In Progress!");
			}
			break;
		}

		return nullptr;
		}, nullptr, "Entry");
	scePthreadJoin(entry_thread, nullptr);

	return SCE_OK;
}

extern "C" int module_stop(size_t args, const void* argp)
{
	ScePthread entry_thread;
	scePthreadCreate(&entry_thread, nullptr, [](void*) -> void* {

		switch (int(id))
		{
			case IW6MP:
			{
				delete GHOMP::DB_FindXAssetHeader_d;
				//delete GHOMP::HECmd_SetText_d;
			}
			break;
			case IW6SP:
			{
				delete GHOSP::DB_FindXAssetHeader_d;
			}
			break;
			case S1MP:
			{
				delete AWMP::DB_FindXAssetHeader_t;
			}
			break;
			case S1SP:
			{
				delete AWSP::DB_FindXAssetHeader_d;
			}
			break;
			case H1MP:
			{
				delete MWRMP::DB_FindXAssetHeader_d;
				//delete MWRMP::HECmd_SetText_d;
			}
			break;
			case H1SP:
			{
				delete MWRSP::DB_FindXAssetHeader_d;
			}
			break;
			case H2:
			{
				delete MW2CR::DB_FindXAssetHeader_d;
			}
			break;
			case S2MP:
			{
				//delete WW2MP::ProcessScrip_t;
			}
			break;
		}

		return nullptr;
		}, nullptr, "Entry");
	scePthreadJoin(entry_thread, nullptr);

	return SCE_OK;
}