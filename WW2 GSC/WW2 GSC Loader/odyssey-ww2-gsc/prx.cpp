#include "stdafx.h"
#include <liborbisrender.h>
#include <png_dec.h>

#include <audioout.h>

#include "app.h"
#include "gsc.h"

#pragma comment(lib, "liborbisrender.a")

extern "C" {
	__declspec(dllexport) void* _ZTINSt8ios_base7failureE = nullptr;
	__declspec(dllexport) void* _ZTVSt5ctypeIcE = nullptr;
}

void run_gui()
{
	static bool has_prev_pressed_key = false;
	bool touchpad_down = liborbisutil::pad::current_pad_data.touchData.touchNum == 2;
	if (touchpad_down && !has_prev_pressed_key)
	{
		has_prev_pressed_key = true;
		app.is_open = !app.is_open;
	}
	else if (!touchpad_down)
	{
		has_prev_pressed_key = false;
	}

	if (!app.is_open)
		return;

	ImGui::Begin("Hello, world!");
	ImGui::Text("This is some useful text.");

	// test osk
	static char buf[256] = { 0 };
	ImGui::InputText("string", buf, 256);

	ImGui::End();
}

void render(int flipIndex)
{
	if (app.update(flipIndex))
	{
		run_gui();

		app.render();
	}
}

extern "C"
{
	int __cdecl module_start(size_t argc, const void* args)
	{
		liborbisutil::thread t([](void*) {
			auto res = MH_Initialize();
			if (res != MH_OK)
			{
				LOG_ERROR("failed to initialize minhook: %d\n", res);
				return;
			}

			//app.init(FunctionImGui | HookFlip | FunctionRenderDebug | SubmitSelf, render);

			gsc::initialize();

			liborbisutil::pad::initialize(liborbisutil::pad::state::read_state, true, [](ScePadData* pad, int num) {

				ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
				if (bd)
					memcpy(&bd->m_sce_pad, pad, sizeof(ScePadData));

				});

			}, "init");

		t.join();

		return SCE_OK;
	}

	int __cdecl module_stop(size_t argc, const void* args)
	{
		liborbisutil::thread t([](void*) {

			liborbisutil::pad::finalize();
			//app.cleanup();

			sceKernelUsleep(500 * 1000);

			MH_Uninitialize();
			}, "release");

		t.join();

		return SCE_OK;
	}
}