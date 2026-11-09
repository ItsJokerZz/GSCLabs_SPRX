#pragma once

#include "stdafx.h"

namespace gsc
{
	enum game_ids
	{
		ODYSSEY_NONE,
		ODYSSEY_IW6MP,
		ODYSSEY_IW6SP,
		ODYSSEY_S1MP,
		ODYSSEY_S1SP,
		ODYSSEY_S2MP,
		ODYSSEY_H1MP,
		ODYSSEY_H1SP,
		ODYSSEY_H2,
		ODYSSEY_IW7
	};

	struct ScriptFile
	{
		const char* name;
		int compressedLen;
		int len;
		int bytecodeLen;
		char* buffer;
		unsigned char* bytecode;
	};

	typedef char* xBlockDataPtr_t;
	struct __attribute__((aligned(8))) StreamPosInfo
	{
		xBlockDataPtr_t pos;
		unsigned int index;
	};

	struct StreamPosGlob
	{
		xBlockDataPtr_t pos;
		xBlockDataPtr_t posArray[9];
		unsigned int posIndex;
		unsigned long long unk;
		StreamPosInfo posStack[64];
	};

	class game
	{
	public:
		game_ids id = ODYSSEY_NONE;

		int script_file_asset_type = 0;

		uintptr_t DB_FindXAssetHeader_address = 0;

		uintptr_t DB_BlockCompressStreamInflate_address = 0;
		uintptr_t DB_BlockCompressStreamDeflate_return = 0;
		std::string memory_script_file = "";
		uintptr_t script_file_address = 0;
		uintptr_t g_script_pos = 0;

		std::string script_directory = "/data/odyssey/";
		std::string replacement_script_name = "";

		void initialize(uintptr_t db_find_xasset_header_addr, uintptr_t db_blockcompress_stream_inflate_addr = 0)
		{
			DB_FindXAssetHeader_address = db_find_xasset_header_addr;
			DB_BlockCompressStreamInflate_address = db_blockcompress_stream_inflate_addr;
		}

		void set_script_directory(const std::string& dir)
		{
			script_directory = dir;

			if (!liborbisutil::filesystem::exists(script_directory))
				liborbisutil::filesystem::create_directory(script_directory, true);
		}

		void set_replacement_script_name(const std::string& name)
		{
			replacement_script_name = name;
		}
	};

	class game_detector
	{
		struct entry
		{
			const char* address;
			game_ids game_id;
		};

		inline static std::vector<entry> detection_table = {
			{reinterpret_cast<const char*>(0xA949BB),	ODYSSEY_S1SP},
			{reinterpret_cast<const char*>(0xB06FDA),	ODYSSEY_IW6SP},
			{reinterpret_cast<const char*>(0xC0B0E7),	ODYSSEY_S1MP},
			{reinterpret_cast<const char*>(0xC251D7),	ODYSSEY_IW6MP},
			{reinterpret_cast<const char*>(0xD46DEE),	ODYSSEY_H2},
			{reinterpret_cast<const char*>(0xD5E106),	ODYSSEY_H1SP},
			{reinterpret_cast<const char*>(0xF991A9),	ODYSSEY_H1MP},
			{reinterpret_cast<const char*>(0x13D7939),	ODYSSEY_S2MP},
			{reinterpret_cast<const char*>(0x1764BAB),	ODYSSEY_IW7},
		};

	public:
		void detect_game();

		game_ids get_detected_game() const { return detected_game; }
	private:
		game_ids detected_game = ODYSSEY_NONE;
	};

	struct script
	{
		std::string name;
		ScriptFile* script_file;
		void* bytecode_ptr;
	};

	struct allocator
	{
		char* ptr;
		char* position;
		size_t size = 0;

		allocator() = default;
		allocator(char* ptr, size_t s)
			: ptr(ptr), position(ptr), size(s) {
		}

		char* allocate(size_t size)
		{
			if (!ptr || !position)
			{
				LOG_ERROR("gsc: allocator not initialized\n");
				return nullptr;
			}

			if (this->position + size > ptr + this->size)
			{
				LOG_ERROR("gsc: allocator out of memory\n");
				return nullptr;
			}

			char* allocated_ptr = this->position;
			this->position += size;
			return allocated_ptr;
		}

		void free()
		{
			if (ptr)
			{
				memset(ptr, 0, reinterpret_cast<size_t>(position) - reinterpret_cast<size_t>(ptr));
				ptr = nullptr;
				position = nullptr;
				size = 0;
			}
		}
	};

	inline allocator script_allocator;
	inline game current_game;
	inline std::unordered_map<std::string, script> scripts;
	inline game_detector detector;

	namespace hooks
	{
		void initialize();
		void terminate();
	}

	void initialize();
}