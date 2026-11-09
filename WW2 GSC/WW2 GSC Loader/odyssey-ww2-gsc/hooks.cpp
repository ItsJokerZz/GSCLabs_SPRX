#include "stdafx.h"
#include <liborbisutil.h>

#include "gsc.h"
#include "filesystem_impl.h"

namespace gsc
{
	namespace hooks
	{
		liborbisutil::hook::manager hook_manager;

		struct script_file_cache
		{
			std::string script_name;
			ScriptFile original_script_file;
			std::unique_ptr<char[]> buffer;
			std::unique_ptr<char[]> bytecode_buffer;
			bool is_cached = false;
		};

		bool found_custom_script = false;
		size_t script_bytecode_size = 0;
		ScriptFile* current_script_file = nullptr;

		std::vector<script_file_cache> script_caches;

		bool IsInLoadingScreen()
		{
			auto connState = *liborbisutil::resolve::get_absolute_address<int*>(0x01FBCC00);
			auto g_virtualLobbyActive = *liborbisutil::resolve::get_absolute_address<unsigned char*>(0x12C6300);

			if (g_virtualLobbyActive)
				return false;

			if (connState == 0)
				return false;

			return (connState < 11);
		}

		ScriptFile* load_custom_script(ScriptFile* script_file, const std::string& script_name)
		{
			std::string script_path = current_game.script_directory + script_name + ".gsc";
			std::ifstream fs(script_path, std::ios::binary | std::ios::ate);
			if (!fs.is_open())
			{
				LOG_ERROR("gsc: failed to open custom script file %s\n", script_path.c_str());
				return script_file;
			}

			if (!found_custom_script)
			{
				script_file_cache cache;
				cache.script_name = script_file->name;

				cache.original_script_file = *script_file;
				cache.buffer = std::make_unique<char[]>(script_file->compressedLen);
				memcpy(cache.buffer.get(), script_file->buffer, script_file->compressedLen);
				cache.bytecode_buffer = std::make_unique<char[]>(script_file->bytecodeLen);
				memcpy(cache.bytecode_buffer.get(), script_file->bytecode, script_file->bytecodeLen);
				cache.is_cached = true;
				script_caches.push_back(std::move(cache));
				found_custom_script = true;
			}

			auto file_size = fs.tellg();
			fs.seekg(0, std::ios::beg);

			char* allocated_buffer = new char[file_size];

			fs.read(allocated_buffer, file_size);
			fs.seekg(0, std::ios::beg);

			while (fs.get() != '\0') {}

			int compressed_len = 0;
			int uncompressed_len = 0;
			int bytecode_len = 0;

			fs.read(reinterpret_cast<char*>(&compressed_len), sizeof(int));
			fs.read(reinterpret_cast<char*>(&uncompressed_len), sizeof(int));
			fs.read(reinterpret_cast<char*>(&bytecode_len), sizeof(int));

			if (compressed_len <= 0 || uncompressed_len <= 0 || bytecode_len <= 0)
			{
				LOG_ERROR("gsc: invalid script lengths in file %s\n", script_path.c_str());
				delete[] allocated_buffer;
				return script_file;
			}

			script_file->compressedLen = compressed_len;
			script_file->len = uncompressed_len;
			script_file->bytecodeLen = bytecode_len;

			script_file->buffer = allocated_buffer + (int)fs.tellg();
			script_file->bytecode = reinterpret_cast<unsigned char*>(script_allocator.allocate(sizeof(unsigned char) * bytecode_len));
			memcpy(script_file->bytecode, allocated_buffer + (int)fs.tellg() + compressed_len, bytecode_len);

			return script_file;
		}

		void* DB_FindXAssetHeader(int type, const char* name, void* a3, void* a4)
		{
			auto res = hook_manager.invoke<ScriptFile*>("DB_FindXAssetHeader", type, name, a3, a4);

			if (type != current_game.script_file_asset_type)
				return res;

			if (strcmp(name, current_game.replacement_script_name.c_str()) != 0)
				return res;

			for (const auto& file : std::filesystem::directory_iterator(current_game.script_directory))
			{
				if (!file.is_regular_file())
					continue;
				auto file_path = file.path().string();
				auto filename = file.path().stem().string();
				if (file_path.find(".gsc") == std::string::npos)
					continue;

				LOG_INFO("gsc: replacing script %s with custom script from %s\n", name, file_path.c_str());

				if (!IsInLoadingScreen())
				{
					LOG_DEBUG("gsc: not in loading screen, skipping custom script load for %s\n", filename.c_str());
					if (found_custom_script && script_caches.size() > 0)
					{
						auto& cache = script_caches.back();
						if (cache.is_cached)
						{
							// restore original script file data
							*res = cache.original_script_file;
							memcpy(res->buffer, cache.buffer.get(), cache.original_script_file.compressedLen);
							memcpy(res->bytecode, cache.bytecode_buffer.get(), cache.original_script_file.bytecodeLen);

							LOG_INFO("gsc: restored original script file for %s\n", filename.c_str());

							script_caches.pop_back();

							if (script_caches.size() == 0)
							{
								found_custom_script = false;
							}
						}
					}

					return res;
				}

				return load_custom_script(reinterpret_cast<ScriptFile*>(res), filename);
			}

			return res;
		}

		void DB_BlockCompressStreamInflate(void* a1, void* a2, void* a3, void* a4)
		{
			hook_manager.invoke<void>("DB_BlockCompressStreamInflate", a1, a2, a3, a4);

			if (__builtin_return_address(0) != (void*)current_game.DB_BlockCompressStreamDeflate_return)
				return;

			if (!IsInLoadingScreen())
				return;

			// Stream global
			StreamPosGlob& g_streamPosGlob = *reinterpret_cast<StreamPosGlob*>(current_game.g_script_pos);

			auto g_scriptFilePtr = *reinterpret_cast<ScriptFile**>(current_game.script_file_address);
			if (!g_scriptFilePtr)
				return;

			// Log name of the script being loaded
			LOG_DEBUG("gsc: loaded script file: %s\n", g_scriptFilePtr->name);

			if (strcmp(g_scriptFilePtr->name, current_game.memory_script_file.data()))
				return;

			auto cached_pos = g_streamPosGlob.pos;
			size_t script_file_size = 0;

			// Custom behavior for script "801" can be added here
			for (const auto& file : std::filesystem::directory_iterator(current_game.script_directory))
			{
				if (!file.is_regular_file())
					continue;

				auto file_path = file.path().string();
				if (file_path.find(".gsc") == std::string::npos)
					continue;

				std::ifstream fs(file_path, std::ios::binary | std::ios::ate);
				if (!fs.is_open())
					continue;

				fs.seekg(0, std::ios::beg);

				while (fs.get() != '\0') {}

				int compressed_len = 0;
				int uncompressed_len = 0;
				int bytecode_len = 0;

				fs.read(reinterpret_cast<char*>(&compressed_len), sizeof(int));
				fs.read(reinterpret_cast<char*>(&uncompressed_len), sizeof(int));
				fs.read(reinterpret_cast<char*>(&bytecode_len), sizeof(int));

				if (compressed_len <= 0 || uncompressed_len <= 0 || bytecode_len <= 0)
					continue;

				// increase position to account for the extra scripts.
				//g_streamPosGlob.pos += bytecode_len;
				script_file_size += bytecode_len;
				script_bytecode_size += g_scriptFilePtr->bytecodeLen;
				current_script_file = g_scriptFilePtr;
				g_scriptFilePtr->bytecodeLen = bytecode_len;

				fs.close();

				LOG_DEBUG("gsc: adjusted stream position for custom script %s, new pos: %d\n", file_path.c_str(), g_streamPosGlob.pos);
			}

			LOG_INFO("gsc: total size of custom scripts added: %d bytes\n", static_cast<int>(script_file_size));

			script_allocator = allocator(reinterpret_cast<char*>(cached_pos + g_scriptFilePtr->bytecodeLen), script_file_size);
		}

		void Load_XAsset(int type, const char* name)
		{
			hook_manager.invoke<void>("Load_XAsset", type, name);

			if (script_bytecode_size)
			{
				auto g_scriptFilePtr = current_script_file;
				if (g_scriptFilePtr)
				{
					g_scriptFilePtr->bytecodeLen = script_bytecode_size;
					script_bytecode_size = 0;
				}
			}
		}

		void initialize()
		{
			if (current_game.DB_FindXAssetHeader_address == 0)
				return;

			hook_manager.create("DB_FindXAssetHeader", current_game.DB_FindXAssetHeader_address, DB_FindXAssetHeader);
			hook_manager.enable("DB_FindXAssetHeader");

			//LOG_INFO("gsc: hooked DB_FindXAssetHeader at 0x%08X\n", current_game.DB_FindXAssetHeader_address);

			if (current_game.DB_BlockCompressStreamInflate_address != 0)
			{
				// Hook DB_BlockCompressStreamInflate if needed
				hook_manager.create("DB_BlockCompressStreamInflate", current_game.DB_BlockCompressStreamInflate_address, DB_BlockCompressStreamInflate);
				hook_manager.enable("DB_BlockCompressStreamInflate");

				LOG_INFO("gsc: hooked DB_BlockCompressStreamInflate at 0x%08X\n", current_game.DB_BlockCompressStreamInflate_address);

				if (current_game.id == ODYSSEY_IW7)
				{
					// Additional hooks for IW7 can be added here
					hook_manager.create("Load_XAsset", 0xF0E4B0, Load_XAsset);
					hook_manager.enable("Load_XAsset");
				}
			}
		}
		void terminate()
		{
			script_allocator.free();
			hook_manager.clear();
		}
	}
}