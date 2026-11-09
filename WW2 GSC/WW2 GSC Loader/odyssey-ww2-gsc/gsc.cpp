#include "gsc.h"
#include <unordered_map>
namespace gsc
{
	void game_detector::detect_game()
	{
		for (const auto& entry : detection_table)
		{
			if (!strcmp(entry.address, "IsMultiplayer"))
			{
				detected_game = entry.game_id;
				return;
			}
		}
	}

	void initialize()
	{
		detector.detect_game();
		current_game.id = detector.get_detected_game();

		switch (current_game.id)
		{
		case ODYSSEY_IW6MP:
			current_game.DB_FindXAssetHeader_address = 0x65FC10;
			current_game.set_script_directory("/data/odyssey/iw6mp/");
			current_game.set_replacement_script_name("1449");
			break;
		case ODYSSEY_IW6SP:
			current_game.DB_FindXAssetHeader_address = 0x5A1250;
			current_game.set_script_directory("/data/odyssey/iw6sp/");
			current_game.set_replacement_script_name("988");
			break;
		case ODYSSEY_S1MP:
			current_game.DB_FindXAssetHeader_address = 0x6A4570;
			current_game.set_script_directory("/data/odyssey/s1mp/");
			current_game.set_replacement_script_name("42918");
			break;
		case ODYSSEY_S1SP:
			current_game.DB_FindXAssetHeader_address = 0x5981F0;
			current_game.set_script_directory("/data/odyssey/s1sp/");
			current_game.set_replacement_script_name("42299");
			break;
		case ODYSSEY_S2MP:
			current_game.DB_FindXAssetHeader_address = 0x723440;
			current_game.DB_BlockCompressStreamInflate_address = 0xC94060;
			current_game.DB_BlockCompressStreamDeflate_return = 0xCD826C;
			current_game.memory_script_file = "801";
			current_game.script_file_address = 0xBDA1F88;
			current_game.g_script_pos = 0xBDCEE48;

			current_game.set_script_directory("/data/odyssey/s2mp/");
			current_game.set_replacement_script_name("1262");
			
			current_game.script_file_asset_type = 57; // ASSET_TYPE_SCRIPT_FILE

			LOG_DEBUG("Set script file asset type to %d for S2MP\n", current_game.script_file_asset_type);
			break;
		case ODYSSEY_H1MP:
			current_game.DB_FindXAssetHeader_address = 0x9B6590;
			current_game.set_script_directory("/data/odyssey/h1mp/");
			current_game.set_replacement_script_name("42928");
			break;
		case ODYSSEY_H1SP:
			current_game.DB_FindXAssetHeader_address = 0x6F4C30;
			current_game.set_script_directory("/data/odyssey/h1sp/");
			current_game.set_replacement_script_name("42309");
			break;
		case ODYSSEY_H2:
			current_game.DB_FindXAssetHeader_address = 0x736480;
			current_game.set_script_directory("/data/odyssey/h2/");
			current_game.set_replacement_script_name("50895");
			break;
		case ODYSSEY_IW7:
			// IW7 not supported yet
			current_game.DB_FindXAssetHeader_address = 0xF39B40;
			current_game.DB_BlockCompressStreamInflate_address = 0x760F30;
			current_game.DB_BlockCompressStreamDeflate_return = 0xF0EB47;
			current_game.memory_script_file = "4013";
			current_game.script_file_address = 0x5A4CB10;
			current_game.g_script_pos = 0x05CF0448;

			current_game.script_file_asset_type = 0x30; // ASSET_TYPE_SCRIPT_FILE

			current_game.set_script_directory("/data/odyssey/iw7/");
			current_game.set_replacement_script_name("2781");
			break;
		default:
			return;
		}

		hooks::initialize();
	}

	void terminate()
	{
		hooks::terminate();
	}
}