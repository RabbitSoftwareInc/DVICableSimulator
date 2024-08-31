// DVI Cable Simulator game, written in C++.
// Graphics API: DirectX 9, DirectX 11 and OpenGL.
//
// Copyright (C) Komok Software. 2024.

#define DIRECTINPUT_VERSION 0x0800

// directx 11 includes
#include <d3d11.h>
#include <d3dx11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")

// sfml and opengl includes
#include <gl/GL.h>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "imgui/imconfig-SFML.h"
#include "imgui/imgui-SFML.h"

// imgui includes
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_stdlib.h"

// audio library
#include "bass/bass.h"

#pragma comment(lib, "bass.lib")

// winapi includes
#include <Windows.h>
#include <thread>
#include <dinput.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <istream>
#include <fstream>
#include <ostream>
#include <codecvt>

// game includes
#include "game/main/assets.h"
#include "game/main/settings.h"
#include "game/main/scenario.h"

#include "game/main/combobox_data.h"
#include "game/main/translation.h"

#include "game/config.h"

// other includes
#include "game/file_features.h"
#include "game/string_features.h"

// discord rpc
#include "discord.h"

// discord rpc data
uint64_t discord_start_timestamp = NULL;

// renderer data
#ifdef DCS_OPENGL
std::wstring game_renderer = L"OpenGL";
#else
std::wstring game_renderer = L"Direct3D 11";
#endif

#ifndef DCS_OPENGL
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;

static ID3D11RenderTargetView* backBuffer = nullptr;
static ID3D11Texture2D* depthStancil = nullptr;
static ID3D11DepthStencilView* depthStancilBuffer = nullptr;
static ID3D11RasterizerState* rasterizer = nullptr;
static D3D11_TEXTURE2D_DESC descDepth;
static D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
static D3D11_VIEWPORT vp;

HHOOK ExistingKeyboardProc;

WINDOWPLACEMENT fullscreenPlacement = { 0 };
WINDOWPLACEMENT windowedPlacement = { 0 };

bool isRender = false;
bool enabelAltEnter = false;

LRESULT WINAPI wndProc(HWND, UINT, WPARAM, LPARAM);

void createDescsTarget();
bool createTargetRender();

bool renderStateEdit(const D3D11_FILL_MODE& fm);
int resizeWindow(SIZE p);

bool initDirectX();

bool hookKeyboardProc(HINSTANCE hinst);
LRESULT CALLBACK keyboardProcLowLevel(int nCode, WPARAM wParam, LPARAM lParam);
int unHookKeyboardProc();

void clear();
#endif

#ifdef DCS_OPENGL
ImTextureID convertGLtoImTexture(GLuint glTextureHandle)
{
	ImTextureID textureID = (ImTextureID)NULL;
	std::memcpy(&textureID, &glTextureHandle, sizeof(GLuint));
	return textureID;
}
#endif

HWND hWnd;
HINSTANCE hInstance;

VideoSettings_t video_settings;
AudioSettings_t audio_settings;
GameSettings_t game_settings;

GameFonts_t game_fonts;
GameMenuData_t game_menu_data;
GameInfo_t game_info;

std::vector<Scenario_t> scenarios;
std::vector<std::wstring> saves;

std::vector<std::string> save_names;
std::vector<std::string> scenario_names;

std::vector<MusicData_t> music;
std::vector<GameLanguage_t> languages;

bool add_start_scene = false;

bool rendered_intro = false;

float intro_alpha_fade_in = 0.0f;
float intro_alpha_fade_out = 0.0f;

bool video_settings_open = false;
bool audio_settings_open = false;
bool game_settings_open = false;

bool button_menu_open = false;
bool game_menu_open = false;
bool history_menu_open = false;
bool save_menu_open = false;

bool select_scenario = false;
bool select_server = false;
bool select_save = false;

bool game_started = false;

bool disable_input_on_scene = false;

int current_scenario_scene = -1;
std::wstring main_character_name = L"";

#ifdef DCS_STORY_GAME
bool advanced_scenes = true;
#else
bool advanced_scenes = false;
#endif

bool scenario_editor = false;
bool editing_scene = false;

int selected_scenario = -1;
int selected_save = -1;

std::wstring current_playing_music = L"NONE";

bool paused_music = false;
bool additional_channel_playing = false;

HSTREAM music_stream = NULL;
HSTREAM additional_stream = NULL;

std::vector<std::wstring> dialogue_history;
bool recorded_dialogue = false;

bool switched_scenario = false;
std::wstring original_scenario_name = L"";

std::wstring dialogue_text_to_render = L"";
int dialogue_added_text_symbols = 0;

float dialogue_text_animation_lerp = 0.0f;

ImVec2 settings_render_position = ImVec2(FLT_MAX, FLT_MAX);

bool started_rpc_thread = false;

bool first_init = false;

bool should_recreate_d3d_device_and_window = false;
bool should_ignore_quit_message_after_window_recreation = false;

std::string std_locale = "default";

const char* LANG(const wchar_t* original = L"", const wchar_t* unused = L"")
{
	for (int i = 0; i < languages.at(game_settings.menu_language).strings.size(); i++)
	{
		if (languages.at(game_settings.menu_language).strings.at(i).original_text == original)
			return utf8(languages.at(game_settings.menu_language).strings.at(i).translation_text.c_str());
	}

	/*if (game_settings.menu_language == 0)
		return utf8(english);
	else if (game_settings.menu_language == 1)
		return utf8(russian);
	else if (game_settings.menu_language == 2)
		return utf8(german);*/

	return "";
}

const wchar_t* LANG_W(const wchar_t* original = L"", const wchar_t* unused = L"")
{
	for (int i = 0; i < languages.at(game_settings.menu_language).strings.size(); i++)
	{
		if (languages.at(game_settings.menu_language).strings.at(i).original_text == original)
			return languages.at(game_settings.menu_language).strings.at(i).translation_text.c_str();
	}

	return L"";
}

float CalcHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool LanguageCombo(const char* label, int* current_item, std::vector<GameLanguage_t> items, int popup_max_height_in_items)
{
	ImGuiContext& g = *GImGui;

	// Call the getter to obtain the preview string which is a parameter to BeginCombo()
	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items.size())
		preview_value = utf8(items.at(*current_item).name.c_str());

	// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
	if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcHeightFromItemCount(popup_max_height_in_items)));

	if (!ImGui::BeginCombo(label, preview_value, ImGuiComboFlags_None))
		return false;

	// Display items
	// FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
	bool value_changed = false;
	for (int i = 0; i < items.size(); i++)
	{
		ImGui::PushID((void*)(intptr_t)i);

		const bool item_selected = (i == *current_item);

		if (ImGui::Selectable(utf8(items.at(i).name.c_str()), item_selected))
		{
			value_changed = true;
			*current_item = i;
		}

		if (item_selected)
			ImGui::SetItemDefaultFocus();

		ImGui::PopID();
	}

	ImGui::EndCombo();
	return value_changed;
}

std::wstring current_date() 
{
	time_t     now = time(0);
	struct tm  tstruct;
	wchar_t       buf[80];
	tstruct = *localtime(&now);
	wcsftime(buf, sizeof(buf), L"%Y-%m-%d", &tstruct);

	return buf;
}

std::wstring current_time()
{
	time_t     now = time(0);
	struct tm  tstruct;
	wchar_t       buf[80];
	tstruct = *localtime(&now);
	wcsftime(buf, sizeof(buf), L"%H.%M.%S", &tstruct);

	return buf;
}

std::wstring get_screenshot_name()
{
#if defined(DCS_CONFIG)
	std::string game = "DVI Cable Simulator";
#elif defined(DDLC_CPP_CONFIG)
	std::wstring game = L"Doki Doki Literature Club";
#endif
	return game + L" " + current_date() + L" " + current_time() + L".png";
}

DWORD WINAPI rpc_update_thread(PVOID r)
{
	int last_scenario = 0;

	bool last_game_state = false;
	bool last_intro_state = false;

	while (true)
	{
		if (last_game_state != game_started || last_intro_state != rendered_intro || last_scenario != selected_scenario)
		{
			if (rendered_intro)
			{
				DiscordRichPresence discordPresence;
				memset(&discordPresence, 0, sizeof(discordPresence));

				discordPresence.startTimestamp = discord_start_timestamp;

				if (game_started)
				{
					std::wstring playing_text = L"";

					if (selected_scenario != -1)
						playing_text += scenarios.at(selected_scenario).file_name;
					else
						playing_text += L"Unknown scenario";

					char details[260];
					wcstombs(details, playing_text.c_str(), 259);

					discordPresence.details = scenario_editor ? "Editing scenario" : "Playing scenario";
					discordPresence.state = details;
				}
				else
					discordPresence.details = "In main menu";

				char image_key[260];
				wcstombs(image_key, game_info.game_rpc_app_logo.c_str(), 259);

				discordPresence.largeImageKey = image_key;

				RPC_UpdatePresence(discordPresence);
			}

			last_game_state = game_started;
			last_intro_state = rendered_intro;
			last_scenario = selected_scenario;
		}

		Sleep(1000);
	}
}

void read_game_fonts_from_file()
{
	wchar_t intro_font[260];
	wchar_t intro_font_size[4];

	wchar_t main_menu_font[260];
	wchar_t main_menu_font_size[4];

	wchar_t dialogue_name_font[260];
	wchar_t dialogue_name_font_size[4];

	wchar_t dialogue_text_font[260];
	wchar_t dialogue_text_font_size[4];

	GetPrivateProfileStringW(L"GameFonts", L"intro_font", L"Verdana.ttf", intro_font, 260, L".\\game\\config\\font_config.ini");
	GetPrivateProfileStringW(L"GameFonts", L"intro_font_size", L"44", intro_font_size, 4, L".\\game\\config\\font_config.ini");

	GetPrivateProfileStringW(L"GameFonts", L"main_menu_font", L"Verdana.ttf", main_menu_font, 260, L".\\game\\config\\font_config.ini");
	GetPrivateProfileStringW(L"GameFonts", L"main_menu_font_size", L"14", main_menu_font_size, 4, L".\\game\\config\\font_config.ini");

	GetPrivateProfileStringW(L"GameFonts", L"dialogue_name_font", L"Verdana.ttf", dialogue_name_font, 260, L".\\game\\config\\font_config.ini");
	GetPrivateProfileStringW(L"GameFonts", L"dialogue_name_font_size", L"16", dialogue_name_font_size, 4, L".\\game\\config\\font_config.ini");

	GetPrivateProfileStringW(L"GameFonts", L"dialogue_text_font", L"Verdana.ttf", dialogue_text_font, 260, L".\\game\\config\\font_config.ini");
	GetPrivateProfileStringW(L"GameFonts", L"dialogue_text_font_size", L"20", dialogue_text_font_size, 4, L".\\game\\config\\font_config.ini");

	game_fonts.intro_font.font_name = std::wstring(intro_font);
	game_fonts.intro_font.font_size = _wtoi(intro_font_size);

	game_fonts.main_menu_font.font_name = std::wstring(main_menu_font);
	game_fonts.main_menu_font.font_size = _wtoi(main_menu_font_size);

	game_fonts.dialogue_name_font.font_name = std::wstring(dialogue_name_font);
	game_fonts.dialogue_name_font.font_size = _wtoi(dialogue_name_font_size);

	game_fonts.dialogue_text_font.font_name = std::wstring(dialogue_text_font);
	game_fonts.dialogue_text_font.font_size = _wtoi(dialogue_text_font_size);
}

void write_game_fonts_to_file(GameFonts_t i)
{
	std::wstringstream intro_font_;
	std::wstringstream intro_font_size_;

	std::wstringstream main_menu_font_;
	std::wstringstream main_menu_font_size_;

	std::wstringstream dialogue_name_font_;
	std::wstringstream dialogue_name_font_size_;

	std::wstringstream dialogue_text_font_;
	std::wstringstream dialogue_text_font_size_;

	intro_font_ << i.intro_font.font_name;
	WritePrivateProfileStringW(L"GameFonts", L"intro_font", intro_font_.str().c_str(), L".\\game\\config\\font_config.ini");
	intro_font_.clear();

	intro_font_size_ << i.intro_font.font_size;
	WritePrivateProfileStringW(L"GameFonts", L"intro_font_size", intro_font_size_.str().c_str(), L".\\game\\config\\font_config.ini");
	intro_font_size_.clear();

	main_menu_font_ << i.main_menu_font.font_name;
	WritePrivateProfileStringW(L"GameFonts", L"main_menu_font", main_menu_font_.str().c_str(), L".\\game\\config\\font_config.ini");
	main_menu_font_.clear();

	main_menu_font_size_ << i.main_menu_font.font_size;
	WritePrivateProfileStringW(L"GameFonts", L"main_menu_font_size", main_menu_font_size_.str().c_str(), L".\\game\\config\\font_config.ini");
	main_menu_font_size_.clear();

	dialogue_name_font_ << i.dialogue_name_font.font_name;
	WritePrivateProfileStringW(L"GameFonts", L"dialogue_name_font", dialogue_name_font_.str().c_str(), L".\\game\\config\\font_config.ini");
	dialogue_name_font_.clear();

	dialogue_name_font_size_ << i.dialogue_name_font.font_size;
	WritePrivateProfileStringW(L"GameFonts", L"dialogue_name_font_size", dialogue_name_font_size_.str().c_str(), L".\\game\\config\\font_config.ini");
	dialogue_name_font_size_.clear();

	dialogue_text_font_ << i.dialogue_text_font.font_name;
	WritePrivateProfileStringW(L"GameFonts", L"dialogue_text_font", dialogue_text_font_.str().c_str(), L".\\game\\config\\font_config.ini");
	dialogue_text_font_.clear();

	dialogue_text_font_size_ << i.dialogue_text_font.font_size;
	WritePrivateProfileStringW(L"GameFonts", L"dialogue_text_font_size", dialogue_text_font_size_.str().c_str(), L".\\game\\config\\font_config.ini");
	dialogue_text_font_size_.clear();
}

void read_game_info_from_file()
{
	wchar_t game_name[260];
	wchar_t game_developer[260];
	wchar_t game_rpc_app_id[260];
	wchar_t game_rpc_app_logo[260];

	GetPrivateProfileStringW(L"GameInfo", L"game_name", L"DVI Cable Simulator", game_name, 260, L".\\game\\game_info.ini");
	GetPrivateProfileStringW(L"GameInfo", L"game_developer", L"Komok Software", game_developer, 260, L".\\game\\game_info.ini");

	GetPrivateProfileStringW(L"GameInfo", L"game_rpc_app_id", L"1256185678920941628", game_rpc_app_id, 260, L".\\game\\game_info.ini");
	GetPrivateProfileStringW(L"GameInfo", L"game_rpc_app_logo", L"logo", game_rpc_app_logo, 260, L".\\game\\game_info.ini");

	game_info.game_name = std::wstring(game_name);
	game_info.game_developer = std::wstring(game_developer);
	game_info.game_rpc_app_id = std::wstring(game_rpc_app_id);
	game_info.game_rpc_app_logo = std::wstring(game_rpc_app_logo);
}

void write_game_info_to_file(GameInfo_t i)
{
	std::wstringstream game_name_;
	std::wstringstream game_developer_;
	std::wstringstream game_rpc_app_id_;
	std::wstringstream game_rpc_app_logo_;

	game_name_ << i.game_name;
	WritePrivateProfileStringW(L"GameInfo", L"game_name", game_name_.str().c_str(), L".\\game\\game_info.ini");
	game_name_.clear();

	game_developer_ << i.game_developer;
	WritePrivateProfileStringW(L"GameInfo", L"game_developer", game_developer_.str().c_str(), L".\\game\\game_info.ini");
	game_developer_.clear();

	game_rpc_app_id_ << i.game_rpc_app_id;
	WritePrivateProfileStringW(L"GameInfo", L"game_rpc_app_id", game_rpc_app_id_.str().c_str(), L".\\game\\game_info.ini");
	game_rpc_app_id_.clear();

	game_rpc_app_logo_ << i.game_rpc_app_logo;
	WritePrivateProfileStringW(L"GameInfo", L"game_rpc_app_logo", game_rpc_app_logo_.str().c_str(), L".\\game\\game_info.ini");
	game_rpc_app_logo_.clear();
}

void read_audio_settings_from_file()
{
	wchar_t music_volume[4];
	wchar_t sound_volume[4];

	GetPrivateProfileStringW(L"AudioSettings", L"music_volume", L"100", music_volume, 4, L".\\game\\config\\audio_settings.ini");
	GetPrivateProfileStringW(L"AudioSettings", L"sound_volume", L"100", sound_volume, 4, L".\\game\\config\\audio_settings.ini");

	audio_settings.music_volume = _wtoi(music_volume);
	audio_settings.sound_volume = _wtoi(sound_volume);
}

void write_audio_settings_to_file(AudioSettings_t i)
{
	std::wstringstream music_volume_;
	std::wstringstream sound_volume_;

	music_volume_ << i.music_volume;
	WritePrivateProfileStringW(L"AudioSettings", L"music_volume", music_volume_.str().c_str(), L".\\game\\config\\audio_settings.ini");
	music_volume_.clear();

	sound_volume_ << i.sound_volume;
	WritePrivateProfileStringW(L"AudioSettings", L"sound_volume", sound_volume_.str().c_str(), L".\\game\\config\\audio_settings.ini");
	sound_volume_.clear();
}

void write_video_settings_to_file(VideoSettings_t s)
{
	std::wstringstream window_mode_;
	std::wstringstream vsync_;
	std::wstringstream render_mode_;
	std::wstringstream texture_quality_;

	window_mode_ << (int)s.screen_mode;
	WritePrivateProfileStringW(L"VideoSettings", L"window_mode", window_mode_.str().c_str(), L".\\game\\config\\video_settings.ini");
	window_mode_.clear();

	vsync_ << (int)s.vsync;
	WritePrivateProfileStringW(L"VideoSettings", L"vsync", vsync_.str().c_str(), L".\\game\\config\\video_settings.ini");
	vsync_.clear();

	render_mode_ << (int)s.render_mode;
	WritePrivateProfileStringW(L"VideoSettings", L"render_mode", render_mode_.str().c_str(), L".\\game\\config\\video_settings.ini");
	render_mode_.clear();

	texture_quality_ << (int)s.texture_quality;
	WritePrivateProfileStringW(L"VideoSettings", L"texture_quality", texture_quality_.str().c_str(), L".\\game\\config\\video_settings.ini");
	texture_quality_.clear();
}

void read_video_settings_from_file()
{
	wchar_t window_mode[4];
	wchar_t vsync[4];
	wchar_t render_mode[4];
	wchar_t texture_quality[4];

	GetPrivateProfileStringW(L"VideoSettings", L"window_mode", L"0", window_mode, 4, L".\\game\\config\\video_settings.ini");
	GetPrivateProfileStringW(L"VideoSettings", L"vsync", L"1", vsync, 4, L".\\game\\config\\video_settings.ini");
	GetPrivateProfileStringW(L"VideoSettings", L"render_mode", L"0", render_mode, 4, L".\\game\\config\\video_settings.ini");
	GetPrivateProfileStringW(L"VideoSettings", L"texture_quality", L"0", texture_quality, 4, L".\\game\\config\\video_settings.ini");

	video_settings.screen_mode = _wtoi(window_mode);

	video_settings.vsync = (bool)_wtoi(vsync);

	video_settings.render_mode = _wtoi(render_mode);
	video_settings.texture_quality = _wtoi(texture_quality);
}

void write_game_settings_to_file(GameSettings_t s)
{
	std::wstringstream autosave_;
	std::wstringstream show_fps_counter_;

	std::wstringstream animated_dialogue_text_;
	std::wstringstream text_animation_speed_;

	std::wstringstream menu_language_;

	autosave_ << (int)s.auto_save;
	WritePrivateProfileStringW(L"GameSettings", L"autosave", autosave_.str().c_str(), L".\\game\\config\\game_settings.ini");
	autosave_.clear();

	show_fps_counter_ << (int)s.show_fps_counter;
	WritePrivateProfileStringW(L"GameSettings", L"show_fps_counter", show_fps_counter_.str().c_str(), L".\\game\\config\\game_settings.ini");
	show_fps_counter_.clear();

	animated_dialogue_text_ << (int)s.animated_dialogue_text;
	WritePrivateProfileStringW(L"GameSettings", L"animated_dialogue_text", animated_dialogue_text_.str().c_str(), L".\\game\\config\\game_settings.ini");
	animated_dialogue_text_.clear();

	text_animation_speed_ << (int)s.text_animation_speed;
	WritePrivateProfileStringW(L"GameSettings", L"text_animation_speed", text_animation_speed_.str().c_str(), L".\\game\\config\\game_settings.ini");
	text_animation_speed_.clear();

	menu_language_ << (int)s.menu_language;
	WritePrivateProfileStringW(L"GameSettings", L"menu_language", menu_language_.str().c_str(), L".\\game\\config\\game_settings.ini");
	menu_language_.clear();
}

void read_game_settings_from_file()
{
	wchar_t autosave[4];
	wchar_t menu_language[4];

	wchar_t animated_dialogue_text[4];
	wchar_t text_animation_speed[4];

	wchar_t show_fps_counter[4];

	GetPrivateProfileStringW(L"GameSettings", L"autosave", L"1", autosave, 4, L".\\game\\config\\game_settings.ini");
	GetPrivateProfileStringW(L"GameSettings", L"show_fps_counter", L"1", show_fps_counter, 4, L".\\game\\config\\game_settings.ini");

	GetPrivateProfileStringW(L"GameSettings", L"animated_dialogue_text", L"1", animated_dialogue_text, 4, L".\\game\\config\\game_settings.ini");
	GetPrivateProfileStringW(L"GameSettings", L"text_animation_speed", L"30", text_animation_speed, 4, L".\\game\\config\\game_settings.ini");

	GetPrivateProfileStringW(L"GameSettings", L"menu_language", L"0", menu_language, 4, L".\\game\\config\\game_settings.ini");

	game_settings.auto_save = (bool)_wtoi(autosave);
	game_settings.show_fps_counter = (bool)_wtoi(show_fps_counter);

	game_settings.animated_dialogue_text = (bool)_wtoi(animated_dialogue_text);
	game_settings.text_animation_speed = _wtoi(text_animation_speed);

	game_settings.menu_language = _wtoi(menu_language);
}

void save_game(std::wstring save_name, GameSave_t save)
{
	std::wstringstream scenario_name_;
	std::wstringstream player_name_;

	std::wstringstream scenario_scene_;

	scenario_name_ << save.scenario_name;
	WritePrivateProfileStringW(L"GameSave", L"scenario_name", scenario_name_.str().c_str(), std::wstring(L".\\game\\saves\\" + save_name + L".savegame").c_str());
	scenario_name_.clear();

	player_name_ << save.player_name;
	WritePrivateProfileStringW(L"GameSave", L"player_name", player_name_.str().c_str(), std::wstring(L".\\game\\saves\\" + save_name + L".savegame").c_str());
	player_name_.clear();

	scenario_scene_ << (int)save.scenario_scene;
	WritePrivateProfileStringW(L"GameSave", L"scenario_scene", scenario_scene_.str().c_str(), std::wstring(L".\\game\\saves\\" + save_name + L".savegame").c_str());
	scenario_scene_.clear();
}

bool load_save(std::wstring save_name, GameSave_t& save)
{
	wchar_t scenario_name[260];
	wchar_t player_name[260];

	wchar_t scenario_scene[256];

	GetPrivateProfileStringW(L"GameSave", L"scenario_name", L"", scenario_name, 260, std::wstring(L".\\game\\saves\\" + save_name).c_str());
	GetPrivateProfileStringW(L"GameSave", L"player_name", L"", player_name, 260, std::wstring(L".\\game\\saves\\" + save_name).c_str());

	GetPrivateProfileStringW(L"GameSave", L"scenario_scene", L"", scenario_scene, 256, std::wstring(L".\\game\\saves\\" + save_name).c_str());

	save.scenario_name = std::wstring(scenario_name);
	save.player_name = std::wstring(player_name);

	if (std::wstring(scenario_scene) == L"" || save.scenario_name == L"" || save.player_name == L"")
		return false;

	save.scenario_scene = _wtoi(scenario_scene);

	return true;
}

void load_music(std::wstring filename)
{
	MusicData_t m;

	m.music_path = std::wstring(L".\\game\\sounds\\" + filename).c_str();
	m.music_name = get_filename_without_ext(filename);

	if (exists(m.music_path))
		music.push_back(m);
}

void load_translation(std::wstring filename)
{
	if (!exists(std::wstring(L".\\game\\translations\\" + filename)))
		return;

	std::wifstream translation_file(std::wstring(L".\\game\\translations\\" + filename).c_str());

	std::vector<std::wstring> lines;

	if (translation_file.is_open())
	{
		std::wstring line;

		while (std::getline(translation_file, line))
			lines.push_back(line);

		translation_file.close();
	}

	for (int i = 0; i < lines.size(); i++)
	{
		if (i == 1 && lines.size() > i)
			std_locale = ws2s(lines.at(i));
	}

	lines.clear();

	translation_file.open(std::wstring(L".\\game\\translations\\" + filename).c_str());

	if (std_locale != "default")
		translation_file.imbue(std::locale(std_locale));

	if (translation_file.is_open())
	{
		std::wstring line;

		while (std::getline(translation_file, line))
			lines.push_back(line);

		translation_file.close();
	}

	GameLanguage_t language;

	language.name = L"INVALIDLANG";
	language.codepage = L"INVALIDCODEPAGE";

	for (int i = 0; i < lines.size(); i++)
	{
		std::vector<std::wstring> splitted_str = split_string(lines.at(i), L'=');

		if (i == 0 && splitted_str.size() == 1)
			language.name = splitted_str.at(0);

		if (i == 1 && splitted_str.size() == 1)
			language.codepage = splitted_str.at(0);

		if (i > 1 && splitted_str.size() == 2)
		{
			TranslatedText_t string;

			string.original_text = splitted_str.at(0);
			string.translation_text = splitted_str.at(1);

			language.strings.push_back(string);
		}
	}

	if (language.name != L"INVALIDLANG" && language.codepage != L"INVALIDCODEPAGE" && language.strings.size() > 0)
		languages.push_back(language);
}

void load_menu_fonts(ImGuiIO& io)
{
#ifdef DCS_OPENGL
	//io.Fonts->ClearFonts();
#endif

	game_fonts.main_menu_font.font_data = io.Fonts->AddFontFromFileTTF(utf8(std::wstring(L".\\game\\fonts\\" + game_fonts.main_menu_font.font_name).c_str()), game_fonts.main_menu_font.font_size, NULL, io.Fonts->GetGlyphRangesCyrillic());
	game_fonts.intro_font.font_data = io.Fonts->AddFontFromFileTTF(utf8(std::wstring(L".\\game\\fonts\\" + game_fonts.intro_font.font_name).c_str()), game_fonts.intro_font.font_size, NULL, io.Fonts->GetGlyphRangesCyrillic());

	game_fonts.dialogue_name_font.font_data = io.Fonts->AddFontFromFileTTF(utf8(std::wstring(L".\\game\\fonts\\" + game_fonts.dialogue_name_font.font_name).c_str()), game_fonts.dialogue_name_font.font_size, NULL, io.Fonts->GetGlyphRangesCyrillic());
	game_fonts.dialogue_text_font.font_data = io.Fonts->AddFontFromFileTTF(utf8(std::wstring(L".\\game\\fonts\\" + game_fonts.dialogue_text_font.font_name).c_str()), game_fonts.dialogue_text_font.font_size, NULL, io.Fonts->GetGlyphRangesCyrillic());

#ifdef DCS_OPENGL
	ImGui::SFML::UpdateFontTexture();
#endif
}

void load_menu_images(ImGuiIO& io)
{
#ifndef DCS_OPENGL
	D3DX11CreateShaderResourceViewFromFileW(g_pd3dDevice, L".\\game\\images\\menu\\logo.png", nullptr, nullptr, &game_menu_data.game_logo.image_data, 0);
	D3DX11CreateShaderResourceViewFromFileW(g_pd3dDevice, L".\\game\\images\\menu\\intro_logo.png", nullptr, nullptr, &game_menu_data.intro_logo.image_data, 0);
	D3DX11CreateShaderResourceViewFromFileW(g_pd3dDevice, L".\\game\\images\\menu\\background.png", nullptr, nullptr, &game_menu_data.main_menu_background.image_data, 0);
#else
	game_menu_data.game_logo.image_data.loadFromFile(".\\game\\images\\menu\\logo.png");
	game_menu_data.intro_logo.image_data.loadFromFile(".\\game\\images\\menu\\intro_logo.png");
	game_menu_data.main_menu_background.image_data.loadFromFile(".\\game\\images\\menu\\background.png");
#endif
}

void load_language_files()
{
	WIN32_FIND_DATAW findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	hFind = FindFirstFileW(L".\\game\\translations\\*", &findData);

	while (FindNextFileW(hFind, &findData) != 0)
	{
		if (get_file_ext(std::wstring(findData.cFileName)) == L".lang")
			load_translation(std::wstring(findData.cFileName));
	}

	FindClose(hFind);
}

void load_music_files()
{
	WIN32_FIND_DATAW findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	hFind = FindFirstFileW(L".\\game\\sounds\\*", &findData);

	while (FindNextFileW(hFind, &findData) != 0)
	{
		if (get_file_ext(std::wstring(findData.cFileName)) == L".mp3" || get_file_ext(std::wstring(findData.cFileName)) == L".ogg")
			load_music(std::wstring(findData.cFileName));
	}

	FindClose(hFind);
}

int find_scenario_index(std::wstring scenario_name)
{
	for (int i = 0; i < scenarios.size(); i++)
	{
		if (scenarios.at(i).file_name == scenario_name)
			return i;
	}

	return -1;
}

#ifndef DCS_OPENGL
ID3D11ShaderResourceView* find_texture(std::wstring texture_name, Scenario_t& scenario)
#else
void* find_texture(std::wstring texture_name, Scenario_t& scenario)
#endif
{
#ifdef DCS_STORY_GAME
	for (int i = 0; i < scenarios.at(find_scenario_index(L"main.sc")).textures.size(); i++)
	{
		if (scenarios.at(find_scenario_index(L"main.sc")).textures.at(i).texture_name == texture_name)
		{
#ifndef DCS_OPENGL
			return scenarios.at(find_scenario_index(L"main.sc")).textures.at(i).texture_data;
#else
			return convertGLtoImTexture(scenarios.at(find_scenario_index(L"main.sc")).textures.at(i).texture_data.getNativeHandle());
#endif
		}
	}
#else
	for (int i = 0; i < scenario.textures.size(); i++)
	{
		if (scenario.textures.at(i).texture_name == texture_name)
		{
#ifndef DCS_OPENGL
			return scenario.textures.at(i).texture_data;
#else
			return convertGLtoImTexture(scenario.textures.at(i).texture_data.getNativeHandle());
#endif
		}
	}
#endif

	return NULL;
}

#ifdef DCS_OPENGL
sf::Texture& find_texture_sf(std::wstring texture_name, Scenario_t& scenario)
{
#ifdef DCS_STORY_GAME
	for (int i = 0; i < scenarios.at(find_scenario_index(L"main.sc")).textures.size(); i++)
	{
		if (scenarios.at(find_scenario_index(L"main.sc")).textures.at(i).texture_name == texture_name)
			return scenarios.at(find_scenario_index(L"main.sc")).textures.at(i).texture_data;
	}
#else
	for (int i = 0; i < scenario.textures.size(); i++)
	{
		if (scenario.textures.at(i).texture_name == texture_name)
			return scenario.textures.at(i).texture_data;
	}
#endif

	return sf::Texture();
}
#endif

MusicData_t find_music(std::wstring music_name)
{
	for (int i = 0; i < music.size(); i++)
	{
		if (music.at(i).music_name == music_name)
			return music.at(i);
	}

	return MusicData_t();
}

void stream_music(std::wstring name, HSTREAM& stream)
{
	MusicData_t music = find_music(name);

	if (music.is_valid_music())
	{
		BASS_ChannelStop(stream);
		BASS_StreamFree(stream);

		stream = BASS_StreamCreateFile(FALSE, music.music_path.c_str(), 0, 0, 0);

		BASS_ChannelFlags(stream, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
		BASS_ChannelPlay(stream, TRUE);
	}
}

void exit_to_main_menu(bool scenario_switch)
{
	game_menu_open = false;
	save_menu_open = false;

	button_menu_open = false;

	video_settings_open = false;
	audio_settings_open = false;
	game_settings_open = false;

	select_scenario = false;
	select_save = false;

	game_started = false;
	disable_input_on_scene = false;

	current_scenario_scene = -1;

	if (!scenario_switch)
	{
		selected_scenario = -1;
		dialogue_history.clear();
	}

	dialogue_text_to_render = L"";
	dialogue_added_text_symbols = 0;

	dialogue_text_animation_lerp = 0.0f;

	recorded_dialogue = false;
}

std::vector<ScenarioTexture_t> load_textures(std::wstring directory)
{
	WIN32_FIND_DATAW findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::wstring full_path = directory;
	std::wstring full_path_with_filter = full_path + L"*";

	std::vector<ScenarioTexture_t> texture_list;

	hFind = FindFirstFileW(full_path_with_filter.c_str(), &findData);

	while (FindNextFileW(hFind, &findData) != 0)
	{
		if (get_file_ext(std::wstring(findData.cFileName)) == L".png")
		{
			ScenarioTexture_t texture;

			texture.texture_name = get_filename_without_ext(std::wstring(findData.cFileName));
			
#ifndef DCS_OPENGL
			D3DX11CreateShaderResourceViewFromFileW(g_pd3dDevice, std::wstring(full_path + std::wstring(findData.cFileName)).c_str(), nullptr, nullptr, &texture.texture_data, 0);
#else
			texture.texture_data.loadFromFile(ws2s(std::wstring(full_path + std::wstring(findData.cFileName))));
#endif

			texture_list.push_back(texture);
		}
	}

	FindClose(hFind);

	return texture_list;
}

std::vector<ScenarioDialogueScene_t> load_scenes(std::wstring path)
{
	std::wifstream scene_file(path.c_str());
	scene_file.imbue(std::locale(std_locale));

	std::vector<std::wstring> lines;

	if (scene_file.is_open())
	{
		std::wstring line;

		while (std::getline(scene_file, line))
			lines.push_back(line);

		scene_file.close();
	}

	std::vector<ScenarioDialogueScene_t> scenes;

	bool found_scene_start = false;
	bool found_scene_end = false;

	int line_number = -1;
	int line_param_read_stage = -1;

	ScenarioDialogueScene_t scene;

	if (wcsstr(GetCommandLineW(), L"-skid1337"))
	{
		scene.button1.button_name = L"NONE";
		scene.button2.button_name = L"NONE";
		scene.button3.button_name = L"NONE";
		scene.button4.button_name = L"NONE";

		scene.background_overlay_texture = L"NONE";
		scene.overlay_texture = L"NONE";

		scene.person1.talking = false;
		scene.person1.person_name = L"NONE";
		scene.person1.person_texture = L"NONE";
		scene.person1.person_texture_left = L"NONE";
		scene.person1.person_texture_right = L"NONE";
		scene.person1.person_texture_head = L"NONE";

		scene.person2.talking = false;
		scene.person2.person_name = L"NONE";
		scene.person2.person_texture = L"NONE";
		scene.person2.person_texture_left = L"NONE";
		scene.person2.person_texture_right = L"NONE";
		scene.person2.person_texture_head = L"NONE";

		scene.person3.talking = false;
		scene.person3.person_name = L"NONE";
		scene.person3.person_texture = L"NONE";
		scene.person3.person_texture_left = L"NONE";
		scene.person3.person_texture_right = L"NONE";
		scene.person3.person_texture_head = L"NONE";

		scene.person4.talking = false;
		scene.person4.person_name = L"NONE";
		scene.person4.person_texture = L"NONE";
		scene.person4.person_texture_left = L"NONE";
		scene.person4.person_texture_right = L"NONE";
		scene.person4.person_texture_head = L"NONE";

		scene.person1.person_name = L"Komok050505";

		scene.person1.talking = true;
		scene.person1.talking_text = L"Hello skid1337!";

		scenes.push_back(scene);

		scene = ScenarioDialogueScene_t();

		scene.button1.button_name = L"NONE";
		scene.button2.button_name = L"NONE";
		scene.button3.button_name = L"NONE";
		scene.button4.button_name = L"NONE";

		scene.background_overlay_texture = L"NONE";
		scene.overlay_texture = L"NONE";

		scene.person1.talking = false;
		scene.person1.person_name = L"NONE";
		scene.person1.person_texture = L"NONE";
		scene.person1.person_texture_left = L"NONE";
		scene.person1.person_texture_right = L"NONE";
		scene.person1.person_texture_head = L"NONE";

		scene.person2.talking = false;
		scene.person2.person_name = L"NONE";
		scene.person2.person_texture = L"NONE";
		scene.person2.person_texture_left = L"NONE";
		scene.person2.person_texture_right = L"NONE";
		scene.person2.person_texture_head = L"NONE";

		scene.person3.talking = false;
		scene.person3.person_name = L"NONE";
		scene.person3.person_texture = L"NONE";
		scene.person3.person_texture_left = L"NONE";
		scene.person3.person_texture_right = L"NONE";
		scene.person3.person_texture_head = L"NONE";

		scene.person4.talking = false;
		scene.person4.person_name = L"NONE";
		scene.person4.person_texture = L"NONE";
		scene.person4.person_texture_left = L"NONE";
		scene.person4.person_texture_right = L"NONE";
		scene.person4.person_texture_head = L"NONE";

		scene.person1.person_name = L"Komok050505";

		scene.person1.talking = true;
		
#ifdef DCS_CONFIG
		scene.person1.talking_text = L"Welcome to DVI Cable Simulator!";
#else
		scene.person1.talking_text = L"Welcome to Doki Doki Literature Club!";
#endif

		scenes.push_back(scene);

		scene = ScenarioDialogueScene_t();
	}

	for (int i = 0; i < lines.size(); i++)
	{
		std::wstring line = lines.at(i);

		if (line == L"[scene_start]")
		{
			found_scene_start = true;
			continue;
		}

		if (line == L"[scene_end]")
		{
			found_scene_end = true;
			scenes.push_back(scene);

			int erased_lines = 0;
			int remaining_lines = i;

			while (erased_lines != i && remaining_lines != 0)
			{
				lines.erase(lines.begin());

				erased_lines++;
				remaining_lines--;
			}

			i = 0;

			found_scene_start = false;
			found_scene_end = false;

			line_number = -1;
			line_param_read_stage = -1;

			scene = ScenarioDialogueScene_t();

			continue;
		}

		if (!found_scene_end && found_scene_start)
		{
			line_param_read_stage++;

			std::vector<std::wstring> splitted_str = split_string(line, L':');

			if (splitted_str.size() >= 3 && line_param_read_stage == 0)
			{
				scene.button1.button_name = L"NONE";
				scene.button2.button_name = L"NONE";
				scene.button3.button_name = L"NONE";
				scene.button4.button_name = L"NONE";

				scene.background_overlay_texture = L"NONE";
				scene.overlay_texture = L"NONE";

				scene.person1.person_texture_left = L"NONE";
				scene.person1.person_texture_right = L"NONE";
				scene.person1.person_texture_head = L"NONE";

				scene.person2.person_texture_left = L"NONE";
				scene.person2.person_texture_right = L"NONE";
				scene.person2.person_texture_head = L"NONE";

				scene.person3.person_texture_left = L"NONE";
				scene.person3.person_texture_right = L"NONE";
				scene.person3.person_texture_head = L"NONE";

				scene.person4.person_texture_left = L"NONE";
				scene.person4.person_texture_right = L"NONE";
				scene.person4.person_texture_head = L"NONE";

				if (advanced_scenes && splitted_str.size() > 3)
				{
					scene.background_texture = splitted_str.at(0);
					scene.background_overlay_texture = splitted_str.at(1);

					scene.background_music = splitted_str.at(2);
					scene.additional_scene_sound = splitted_str.at(3);
				}
				else
				{
					scene.background_texture = splitted_str.at(0);
					scene.background_music = splitted_str.at(1);
					scene.additional_scene_sound = splitted_str.at(2);
				}
			}
			else if (splitted_str.size() >= 3 && line_param_read_stage == 1)
			{
				scene.person1.person_name = splitted_str.at(0);
				scene.person1.person_texture = splitted_str.at(1);

				scene.person1.talking_text = splitted_str.at(2);

				if (advanced_scenes && splitted_str.size() > 5)
				{
					scene.person1.person_texture_left = splitted_str.at(3);
					scene.person1.person_texture_right = splitted_str.at(4);
					scene.person1.person_texture_head = splitted_str.at(5);
				}

				if (scene.person1.talking_text == L"NONE")
					scene.person1.talking = false;
				else
					scene.person1.talking = true;
			}
			else if (splitted_str.size() >= 3 && line_param_read_stage == 2)
			{
				scene.person2.person_name = splitted_str.at(0);
				scene.person2.person_texture = splitted_str.at(1);

				scene.person2.talking_text = splitted_str.at(2);

				if (advanced_scenes && splitted_str.size() > 5)
				{
					scene.person2.person_texture_left = splitted_str.at(3);
					scene.person2.person_texture_right = splitted_str.at(4);
					scene.person2.person_texture_head = splitted_str.at(5);
				}

				if (scene.person2.talking_text == L"NONE")
					scene.person2.talking = false;
				else
					scene.person2.talking = true;
			}
			else if (splitted_str.size() >= 3 && line_param_read_stage == 3)
			{
				scene.person3.person_name = splitted_str.at(0);
				scene.person3.person_texture = splitted_str.at(1);

				scene.person3.talking_text = splitted_str.at(2);

				if (advanced_scenes && splitted_str.size() > 5)
				{
					scene.person3.person_texture_left = splitted_str.at(3);
					scene.person3.person_texture_right = splitted_str.at(4);
					scene.person3.person_texture_head = splitted_str.at(5);
				}

				if (scene.person3.talking_text == L"NONE")
					scene.person3.talking = false;
				else
					scene.person3.talking = true;
			}
			else if (splitted_str.size() >= 3 && line_param_read_stage == 4)
			{
				scene.person4.person_name = splitted_str.at(0);
				scene.person4.person_texture = splitted_str.at(1);

				scene.person4.talking_text = splitted_str.at(2);

				if (advanced_scenes && splitted_str.size() > 5)
				{
					scene.person4.person_texture_left = splitted_str.at(3);
					scene.person4.person_texture_right = splitted_str.at(4);
					scene.person4.person_texture_head = splitted_str.at(5);
				}

				if (scene.person4.talking_text == L"NONE")
					scene.person4.talking = false;
				else
					scene.person4.talking = true;
			}
			else if (splitted_str.size() == 1 && line_param_read_stage == 5)
			{
				scene.main_character.talking_text = splitted_str.at(0);

				if (scene.main_character.talking_text == L"NONE")
					scene.main_character.talking = false;
				else
					scene.main_character.talking = true;
			}
			else if (splitted_str.size() == 2 && line_param_read_stage == 6)
			{
				scene.button1.button_name = splitted_str.at(0);
				scene.button1.button_scenario_to_load = splitted_str.at(1);
			}
			else if (splitted_str.size() == 2 && line_param_read_stage == 7)
			{
				scene.button2.button_name = splitted_str.at(0);
				scene.button2.button_scenario_to_load = splitted_str.at(1);
			}
			else if (splitted_str.size() == 2 && line_param_read_stage == 8)
			{
				scene.button3.button_name = splitted_str.at(0);
				scene.button3.button_scenario_to_load = splitted_str.at(1);
			}
			else if (splitted_str.size() == 2 && line_param_read_stage == 9)
			{
				scene.button4.button_name = splitted_str.at(0);
				scene.button4.button_scenario_to_load = splitted_str.at(1);
			}
			else if (splitted_str.size() == 1 && line_param_read_stage == 10 && advanced_scenes)
			{
				scene.overlay_texture = splitted_str.at(0);
			}
		}
	}

	return scenes;
}

void create_new_scenario(std::wstring name)
{
	if (exists(L".\\game\\scenarios\\" + name + L".sc"))
		return;

	std::wofstream new_scenario(L".\\game\\scenarios\\" + name + L".sc");
	new_scenario.imbue(std::locale(std_locale));

	new_scenario << L"[scene_start]" << std::endl;

	if (advanced_scenes)
		new_scenario << L"NONE:NONE:NONE:NONE" << std::endl;
	else
		new_scenario << L"NONE:NONE:NONE" << std::endl;

	if (advanced_scenes)
	{
		new_scenario << L"NONE:NONE:NONE:NONE:NONE:NONE" << std::endl;
		new_scenario << L"NONE:NONE:NONE:NONE:NONE:NONE" << std::endl;
		new_scenario << L"NONE:NONE:NONE:NONE:NONE:NONE" << std::endl;
		new_scenario << L"NONE:NONE:NONE:NONE:NONE:NONE" << std::endl;
	}
	else
	{
		new_scenario << L"NONE:NONE:NONE" << std::endl;
		new_scenario << L"NONE:NONE:NONE" << std::endl;
		new_scenario << L"NONE:NONE:NONE" << std::endl;
		new_scenario << L"NONE:NONE:NONE" << std::endl;
	}

	new_scenario << L"NONE" << std::endl;

	new_scenario << L"NONE:NONE" << std::endl;
	new_scenario << L"NONE:NONE" << std::endl;
	new_scenario << L"NONE:NONE" << std::endl;
	new_scenario << L"NONE:NONE" << std::endl;

	if (advanced_scenes)
		new_scenario << L"NONE" << std::endl;

	new_scenario << L"[scene_end]" << std::endl;

	new_scenario.close();
}

void save_scenario_data(Scenario_t& scenario)
{
	std::wofstream scenario_file(scenario.file_path);
	scenario_file.imbue(std::locale(std_locale));

	for (int i = 0; i < scenario.scenes.size(); i++)
	{
		ScenarioDialogueScene_t& scene = scenario.scenes.at(i);

		scenario_file << L"[scene_start]" << std::endl;

		scenario_file << scene.background_texture << (advanced_scenes ? L":" + scene.background_overlay_texture : L"") << L":" << scene.background_music << L":" << scene.additional_scene_sound << std::endl;

		if (advanced_scenes)
		{
			scenario_file << scene.person1.person_name << L":" << scene.person1.person_texture << L":" << scene.person1.talking_text << L":" << scene.person1.person_texture_left << L":" << scene.person1.person_texture_right << L":" << scene.person1.person_texture_head << std::endl;
			scenario_file << scene.person2.person_name << L":" << scene.person2.person_texture << L":" << scene.person2.talking_text << L":" << scene.person2.person_texture_left << L":" << scene.person2.person_texture_right << L":" << scene.person2.person_texture_head << std::endl;
			scenario_file << scene.person3.person_name << L":" << scene.person3.person_texture << L":" << scene.person3.talking_text << L":" << scene.person3.person_texture_left << L":" << scene.person3.person_texture_right << L":" << scene.person3.person_texture_head << std::endl;
			scenario_file << scene.person4.person_name << L":" << scene.person4.person_texture << L":" << scene.person4.talking_text << L":" << scene.person4.person_texture_left << L":" << scene.person4.person_texture_right << L":" << scene.person4.person_texture_head << std::endl;
		}
		else
		{
			scenario_file << scene.person1.person_name << L":" << scene.person1.person_texture << L":" << scene.person1.talking_text << std::endl;
			scenario_file << scene.person2.person_name << L":" << scene.person2.person_texture << L":" << scene.person2.talking_text << std::endl;
			scenario_file << scene.person3.person_name << L":" << scene.person3.person_texture << L":" << scene.person3.talking_text << std::endl;
			scenario_file << scene.person4.person_name << L":" << scene.person4.person_texture << L":" << scene.person4.talking_text << std::endl;
		}

		scenario_file << scene.main_character.talking_text << std::endl;

		scenario_file << scene.button1.button_name << L":" << scene.button1.button_scenario_to_load << std::endl;
		scenario_file << scene.button2.button_name << L":" << scene.button2.button_scenario_to_load << std::endl;
		scenario_file << scene.button3.button_name << L":" << scene.button3.button_scenario_to_load << std::endl;
		scenario_file << scene.button4.button_name << L":" << scene.button4.button_scenario_to_load << std::endl;

		scenario_file << scene.overlay_texture << std::endl;

		scenario_file << L"[scene_end]" << std::endl;
	}

	scenario_file.close();
}

void load_scenario_data(int scenario_idx)
{
	if (scenario_idx < 0 || scenario_idx >= scenarios.size())
		return;

	Scenario_t& scenario = scenarios.at(scenario_idx);

	if (scenario.loaded)
		return;

	scenario.textures = load_textures(scenario.textures_dir);
	scenario.scenes = load_scenes(scenario.file_path);

	scenario.loaded = true;
}

std::vector<Scenario_t> load_scenarios(std::wstring directory, std::wstring texture_folder, std::wstring music_folder)
{
	WIN32_FIND_DATAW findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::wstring full_path = directory + L"\\";
	std::wstring full_path_with_filter = full_path + L"*";

	std::vector<Scenario_t> scenario_list;

	hFind = FindFirstFileW(full_path_with_filter.c_str(), &findData);

	while (FindNextFileW(hFind, &findData) != 0)
	{
		Scenario_t scenario;

		scenario.file_name = std::wstring(findData.cFileName);
		scenario.file_path = std::wstring(full_path + findData.cFileName);
		scenario.textures_dir = texture_folder + L"\\" + std::wstring(get_filename_without_ext(scenario.file_name)) + L"\\";
		scenario.music_dir = music_folder + L"\\" + std::wstring(get_filename_without_ext(scenario.file_name)) + L"\\";

		scenario.loaded = false;

		if (get_file_ext(scenario.file_name) == L".sc")
			scenario_list.push_back(scenario);
	}

	FindClose(hFind);

	return scenario_list;
}

void load_game_assets(ImGuiIO& io)
{
	load_menu_fonts(io);
	load_menu_images(io);

	load_music_files();

	for (int i = 0; i < scenarios.size(); i++)
		load_scenario_data(i);
}

void reload_game_assets(ImGuiIO& io)
{
	load_menu_fonts(io);
	load_menu_images(io);
}

void unload_game_images_and_textures()
{
	for (int i = 0; i < scenarios.size(); i++)
	{
#ifndef DCS_OPENGL
		for (int c = 0; c < scenarios.at(i).textures.size(); c++)
			scenarios.at(i).textures.at(c).texture_data->Release();
#endif

		scenarios.at(i).scenes.clear();
		scenarios.at(i).textures.clear();

		scenarios.at(i).loaded = false;
	}
}

void unload_music_data()
{
	BASS_ChannelStop(music_stream);
	BASS_StreamFree(music_stream);

	BASS_ChannelStop(additional_stream);
	BASS_StreamFree(additional_stream);
}

void unload_game_assets()
{
	unload_game_images_and_textures();
	unload_music_data();
}

void audio_settings_menu()
{
	static AudioSettings_t new_settings = audio_settings;

	ImVec2 size = ImVec2(250, 145);
	ImVec2 position = settings_render_position;

	ImGuiWindowFlags flags = ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse;

	if (position.x != FLT_MAX)
		flags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar;

	ImGui::SetNextWindowSize(size);

	if (position.x != FLT_MAX)
		ImGui::SetNextWindowPos(position);

	ImGui::Begin(LANG(L"Audio settings", L"Настройки звука"), (bool*)0, flags);

	ImGui::Text(LANG(L"Music volume", L"Громкость музыки"));
	ImGui::SliderInt("##MUSICVOL", &new_settings.music_volume, 0, 100);

	ImGui::Text(LANG(L"Sound volume", L"Громкость звуков"));
	ImGui::SliderInt("##SOUNDVOL", &new_settings.sound_volume, 0, 100);

	if (ImGui::Button(LANG(L"Apply", L"Применить")))
	{
		write_audio_settings_to_file(new_settings);
		audio_settings = new_settings;
	}

	ImGui::SameLine();

	if (ImGui::Button(LANG(L"Cancel", L"Отмена")))
		new_settings = audio_settings;

	ImGui::SameLine();

	if (ImGui::Button(LANG(L"Close", L"Закрыть")))
		audio_settings_open = false;

	ImGui::End();

	if (settings_render_position.x != FLT_MAX)
		settings_render_position.y += size.y + 10;
}

void game_settings_menu()
{
	static GameSettings_t new_settings = game_settings;

	ImVec2 size = ImVec2(250, 217);
	ImVec2 position = settings_render_position;

	ImGuiWindowFlags flags = ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse;

	if (position.x != FLT_MAX)
		flags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar;

	ImGui::SetNextWindowSize(size);

	if (position.x != FLT_MAX)
		ImGui::SetNextWindowPos(position);

	ImGui::Begin(LANG(L"Game settings", L"Настройки игры"), (bool*)0, flags);

	ImGui::Checkbox(LANG(L"Auto save", L"Автосохранение"), &new_settings.auto_save);
	ImGui::Checkbox(LANG(L"Show FPS counter", L"Показывать счетчик FPS"), &new_settings.show_fps_counter);

	ImGui::Checkbox(LANG(L"Animated dialogue text", L"Анимированный текст диалогов"), &new_settings.animated_dialogue_text);

	ImGui::Text(LANG(L"Text animation speed", L"Скорость анимации текста"));
	ImGui::SliderInt("##Text animation speed", &new_settings.text_animation_speed, 1, 100);

	ImGui::Text(LANG(L"Menu language", L"Язык меню"));
	LanguageCombo("##Menu language", &new_settings.menu_language, languages, languages.size());

	if (ImGui::Button(LANG(L"Apply", L"Применить")))
	{
		write_game_settings_to_file(new_settings);
		game_settings = new_settings;
	}

	ImGui::SameLine();

	if (ImGui::Button(LANG(L"Cancel", L"Отмена")))
		new_settings = game_settings;

	ImGui::SameLine();

	if (ImGui::Button(LANG(L"Close", L"Закрыть")))
		game_settings_open = false;

	ImGui::End();

	if (settings_render_position.x != FLT_MAX)
		settings_render_position.y += size.y + 10;
}

void video_settings_menu()
{
	static VideoSettings_t new_settings = video_settings;

	ImVec2 size = ImVec2(250, 210);
	ImVec2 position = settings_render_position;

	ImGuiWindowFlags flags = ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse;

	if (position.x != FLT_MAX)
		flags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar;

	ImGui::SetNextWindowSize(size);

	if (position.x != FLT_MAX)
		ImGui::SetNextWindowPos(position);

	ImGui::Begin(LANG(L"Video settings", L"Настройки видео"), (bool*)0, flags);

	ImGui::Text(LANG(L"Render mode", L"Режим рендеринга"));
	ImGui::Combo("##Render mode", &new_settings.render_mode, RenderMode, ARRAYSIZE(RenderMode));

	ImGui::Text(LANG(L"Window mode", L"Режим окна"));
	ImGui::Combo("##Window mode", &new_settings.screen_mode, ScreenMode, ARRAYSIZE(ScreenMode));

	ImGui::Checkbox(LANG(L"VSync", L"Верт. синхронизация"), &new_settings.vsync);

	ImGui::Text(LANG(L"Texture quality", L"Качество текстур"));
	ImGui::Combo("##Texture quality", &new_settings.texture_quality, TextureQuality, ARRAYSIZE(TextureQuality));

	if (ImGui::Button(LANG(L"Apply", L"Применить")))
	{
		write_video_settings_to_file(new_settings);
		
		if (video_settings.screen_mode != 0)
			MessageBoxW(GetForegroundWindow(), LANG_W(L"Restart game to apply settings", L"Перезапустите игру для применения настроек"), LANG_W(L"Success", L"Успешно"), 0);
	}

	ImGui::SameLine();

	if (ImGui::Button(LANG(L"Cancel", L"Отмена")))
		new_settings = video_settings;

	ImGui::SameLine();

	if (ImGui::Button(LANG(L"Close", L"Закрыть")))
		video_settings_open = false;

	ImGui::End();

	if (settings_render_position.x != FLT_MAX)
		settings_render_position.y += size.y + 10;
}

void save_menu(Scenario_t& scenario)
{
	ImVec2 size = ImVec2(260, 105);

	ImGui::SetNextWindowSize(size);
	ImGui::Begin("##GAMESAVEMENU", &save_menu_open, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);

	static char save_name[260];
	ImGui::Text(LANG(L"Enter save name", L"Введите имя сохранения"));

	ImGui::SetNextItemWidth(240);
	ImGui::InputText("##Save name input", save_name, 260);

	if (ImGui::Button(LANG(L"Save", L"Сохранить"), ImVec2(240, 20)))
	{
		wchar_t save_name_w[260];
		mbstowcs(save_name_w, save_name, 259);

		GameSave_t save;

		save.scenario_name = scenario.file_name;
		save.player_name = main_character_name;

		save.scenario_scene = current_scenario_scene;

		save_game(std::wstring(save_name_w), save);

		saves = get_directory_files_name_w(L".\\game\\saves", L".savegame");
		save_names = get_directory_files_name(".\\game\\saves", ".savegame");

		save_menu_open = false;
	}

	ImGui::End();
}

void main_game_menu(Scenario_t& scenario)
{
	ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, ImColor(20, 20, 20, 135));

	ImVec2 size = ImVec2(250, 600);
	ImVec2 history_size = ImVec2(size.x * 2.5f, size.y);

	if (scenario_editor)
		history_size = ImVec2(FLT_MAX, FLT_MAX);

	ImVec2 position = ImVec2(ImGui::GetIO().DisplaySize.x / 2 - (size.x + (history_size.x != FLT_MAX ? history_size.x + 10 : 0)) / 2, ImGui::GetIO().DisplaySize.y / 2 - size.y / 2);
	ImVec2 history_position = ImVec2(position.x + size.x + 10, position.y);

	settings_render_position = ImVec2(FLT_MAX, FLT_MAX);

	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowPos(position);
	ImGui::Begin("##GAMEMAINMENU", (bool*)0, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

	ImGui::Image(game_menu_data.game_logo.image_data, ImVec2(230, 120));

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button(LANG(L"Back to game", L"Вернуться к игре"), ImVec2(230, 20)))
	{
		game_menu_open = false;
		save_menu_open = false;
	}

	if (!scenario_editor)
	{
		if (ImGui::Button(LANG(L"Save game", L"Сохранить игру"), ImVec2(230, 20)))
			save_menu_open = !save_menu_open;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button(LANG(L"Video settings", L"Настройки видео"), ImVec2(230, 20)))
		video_settings_open = !video_settings_open;

	if (ImGui::Button(LANG(L"Audio settings", L"Настройки звука"), ImVec2(230, 20)))
		audio_settings_open = !audio_settings_open;

	if (ImGui::Button(LANG(L"Game settings", L"Настройки игры"), ImVec2(230, 20)))
		game_settings_open = !game_settings_open;

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button(LANG(L"Exit to main menu", L"Выйти в главное меню"), ImVec2(230, 20)))
		exit_to_main_menu(false);

	if (ImGui::Button(LANG(L"Quit game", L"Выход из игры"), ImVec2(230, 20)))
		exit(0);

	ImGui::End();

	if (history_size.x != FLT_MAX && history_size.y != FLT_MAX)
	{
		ImGui::SetNextWindowSize(history_size);
		ImGui::SetNextWindowPos(history_position);
		ImGui::Begin("##DIALOGUE HISTORY", (bool*)0, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

		ImGui::Text(LANG(L"History", L"История"));

		for (int i = 0; i < dialogue_history.size(); i++)
			ImGui::TextWrapped(utf8(dialogue_history.at(i).c_str()));

		ImGui::End();
	}

	if (save_menu_open)
		save_menu(scenario);
}

void texture_selector(const wchar_t* name, Scenario_t& scenario, std::wstring& texture)
{
	static std::string search_buf = "";

	ImGui::Text(utf8(name));

	ImGui::SetNextItemWidth(200);
	ImGui::InputTextWithHint(std::string(std::string("##SEARCH") + ws2s(name)).c_str(), LANG(L"Search", L"Поиск"), &search_buf);

	ImGui::SetNextItemWidth(200);
	if (ImGui::BeginListBox(std::string(std::string("##") + ws2s(name)).c_str()))
	{
		for (int i = 0; i < scenario.textures.size() + 1; i++)
		{
			if (i == 0)
			{
				if (ImGui::Selectable("None", texture == L"NONE"))
					texture = L"NONE";
			}
			else
			{
				if ((search_buf == "" || strstr(ws2s(scenario.textures.at(i - 1).texture_name).c_str(), search_buf.c_str())) && ImGui::Selectable(ws2s(scenario.textures.at(i - 1).texture_name).c_str(), texture == scenario.textures.at(i - 1).texture_name))
					texture = scenario.textures.at(i - 1).texture_name;
			}
		}

		ImGui::EndListBox();
	}
}

void scenario_selector(const wchar_t* name, std::wstring& scenario, int idx = 0)
{
	static std::string search_buf = "";

	ImGui::Text(utf8(name));

	ImGui::SetNextItemWidth(200);
	ImGui::InputTextWithHint(std::string(std::string("##SEARCH") + ws2s(name) + std::to_string(idx)).c_str(), LANG(L"Search", L"Поиск"), &search_buf);

	ImGui::SetNextItemWidth(200);
	if (ImGui::BeginListBox(std::string(std::string("##") + ws2s(name) + std::to_string(idx)).c_str()))
	{
		for (int i = 0; i < scenario_names.size() + 1; i++)
		{
			if (i == 0)
			{
				if (ImGui::Selectable("None", scenario == L"NONE"))
					scenario = L"NONE";
			}
			else
			{
				if ((search_buf == "" || strstr(scenario_names.at(i - 1).c_str(), search_buf.c_str())) && ImGui::Selectable(scenario_names.at(i - 1).c_str(), scenario == s2ws(scenario_names.at(i - 1))))
					scenario = s2ws(scenario_names.at(i - 1));
			}
		}

		ImGui::EndListBox();
	}
}

void sound_selector(const wchar_t* name, Scenario_t& scenario, std::wstring& sound)
{
	static std::string search_buf = "";

	ImGui::Text(utf8(name));

	ImGui::SetNextItemWidth(200);
	ImGui::InputTextWithHint(std::string(std::string("##SEARCH") + ws2s(name)).c_str(), LANG(L"Search", L"Поиск"), &search_buf);

	ImGui::SetNextItemWidth(200);
	if (ImGui::BeginListBox(std::string(std::string("##") + ws2s(name)).c_str()))
	{
		for (int i = 0; i < music.size() + 1; i++)
		{
			if (i == 0)
			{
				if (ImGui::Selectable("None", sound == L"NONE"))
					sound = L"NONE";
			}
			else
			{
				if ((search_buf == "" || strstr(ws2s(music.at(i - 1).music_name).c_str(), search_buf.c_str())) && ImGui::Selectable(ws2s(music.at(i - 1).music_name).c_str(), sound == music.at(i - 1).music_name))
					sound = music.at(i - 1).music_name;
			}
		}

		ImGui::EndListBox();
	}
}

void main_game()
{
	ImGui::PushFont(game_fonts.main_menu_font.font_data);

	Scenario_t& scenario = scenarios.at(selected_scenario);

	if (scenario.scenes.size() > 0 && current_scenario_scene == -1)
		current_scenario_scene = 0;

	ScenarioDialogueScene_t& scene = scenario.scenes.at(current_scenario_scene);

	if (scenario_editor)
	{
		ImGui::SetNextWindowSize(ImVec2(250, 495));
		ImGui::Begin(LANG(L"Scenario", L"Сценарий"), (bool*)0, ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse);

		ImGui::Text(LANG(L"Scenes", L"Сцены"));

		ImGui::SetNextItemWidth(230);
		if (ImGui::BeginListBox("##Scene selector"))
		{
			for (int i = 0; i < scenario.scenes.size(); i++)
			{
				if (ImGui::Selectable(std::string("scene" + std::to_string(i)).c_str(), current_scenario_scene == i))
				{
					scenario.scenes.at(i).main_character.talking = scenario.scenes.at(i).main_character.talking_text != L"NONE";

					scenario.scenes.at(i).person1.talking = scenario.scenes.at(i).person1.talking_text != L"NONE";
					scenario.scenes.at(i).person2.talking = scenario.scenes.at(i).person2.talking_text != L"NONE";
					scenario.scenes.at(i).person3.talking = scenario.scenes.at(i).person3.talking_text != L"NONE";
					scenario.scenes.at(i).person4.talking = scenario.scenes.at(i).person4.talking_text != L"NONE";

					BASS_ChannelStop(additional_stream);
					BASS_StreamFree(additional_stream);

					additional_channel_playing = false;
					recorded_dialogue = false;

					dialogue_text_to_render = L"";
					dialogue_added_text_symbols = 0;

					dialogue_text_animation_lerp = 0.0f;

					current_scenario_scene = i;
				}
			}

			ImGui::EndListBox();
		}

		if (ImGui::Button(LANG(L"Create new scene", L"Создать новую сцену"), ImVec2(230, 20)))
		{
			ScenarioDialogueScene_t scene;

			scene.button1.button_name = L"NONE";
			scene.button2.button_name = L"NONE";
			scene.button3.button_name = L"NONE";
			scene.button4.button_name = L"NONE";

			scene.background_overlay_texture = L"NONE";
			scene.overlay_texture = L"NONE";
			scene.background_music = L"NONE";
			scene.additional_scene_sound = L"NONE";

			scene.person1.talking = false;
			scene.person1.talking_text = L"NONE";
			scene.person1.person_name = L"NONE";
			scene.person1.person_texture = L"NONE";
			scene.person1.person_texture_left = L"NONE";
			scene.person1.person_texture_right = L"NONE";
			scene.person1.person_texture_head = L"NONE";

			scene.person2.talking = false;
			scene.person2.talking_text = L"NONE";
			scene.person2.person_name = L"NONE";
			scene.person2.person_texture = L"NONE";
			scene.person2.person_texture_left = L"NONE";
			scene.person2.person_texture_right = L"NONE";
			scene.person2.person_texture_head = L"NONE";

			scene.person3.talking = false;
			scene.person3.talking_text = L"NONE";
			scene.person3.person_name = L"NONE";
			scene.person3.person_texture = L"NONE";
			scene.person3.person_texture_left = L"NONE";
			scene.person3.person_texture_right = L"NONE";
			scene.person3.person_texture_head = L"NONE";

			scene.person4.talking = false;
			scene.person4.talking_text = L"NONE";
			scene.person4.person_name = L"NONE";
			scene.person4.person_texture = L"NONE";
			scene.person4.person_texture_left = L"NONE";
			scene.person4.person_texture_right = L"NONE";
			scene.person4.person_texture_head = L"NONE";

			scene.main_character.talking = false;
			scene.main_character.talking_text = L"NONE";

			scenario.scenes.push_back(scene);
		}

		if (ImGui::Button(LANG(L"Create scene duplicate", L"Создать дубликат сцены"), ImVec2(230, 20)))
		{
			ScenarioDialogueScene_t new_scene = scene;

			new_scene.main_character.talking = new_scene.main_character.talking_text != L"NONE";

			new_scene.person1.talking = new_scene.person1.talking_text != L"NONE";
			new_scene.person2.talking = new_scene.person2.talking_text != L"NONE";
			new_scene.person3.talking = new_scene.person3.talking_text != L"NONE";
			new_scene.person4.talking = new_scene.person4.talking_text != L"NONE";

			scenario.scenes.push_back(new_scene);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button(LANG(L"Edit scene", L"Редактировать сцену"), ImVec2(230, 20)))
			editing_scene = true;

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button(LANG(L"Delete scene", L"Удалить сцену"), ImVec2(230, 20)))
		{
			if (current_scenario_scene >= 0 && current_scenario_scene < scenario.scenes.size() - 1)
			{
				if (scenario.scenes.size() > 1)
				{
					scenario.scenes.erase(scenario.scenes.begin() + current_scenario_scene);

					dialogue_text_to_render = L"";
					dialogue_added_text_symbols = 0;

					dialogue_text_animation_lerp = 0.0f;
				}
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button(LANG(L"Save scenario", L"Сохранить сценарий"), ImVec2(230, 20)))
			save_scenario_data(scenario);

		ImGui::End();

		if (editing_scene)
		{
			ImGui::SetNextWindowSize(ImVec2(600, 400));
			ImGui::Begin(LANG(L"Scene editor", L"Редактор сцены"), &editing_scene, ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse);

			if (ImGui::BeginTabBar("##SCENEEDITORTABS"))
			{
				if (ImGui::BeginTabItem(LANG(L"Main", L"Основное")))
				{
					if (advanced_scenes)
						ImGui::Columns(6);
					else
						ImGui::Columns(4);

					std::string dialogue_text = ws2s(scene.main_character.talking_text);

					if (dialogue_text == "NONE")
						dialogue_text = "";

					ImGui::Text(LANG(L"Main character text", L"Текст главного персонажа"));
					ImGui::SetNextItemWidth(200);

					if (ImGui::InputText("##MAINCHARACTERDIALOGUETEXT", &dialogue_text))
						scene.main_character.talking_text = s2ws(dialogue_text);

					if (scene.main_character.talking_text == L"")
						scene.main_character.talking_text = L"NONE";

					if (scene.main_character.talking_text != L"NONE")
						scene.main_character.talking = true;

					ImGui::NextColumn();
					texture_selector(LANG_W(L"Background texture", L"Фоновая текстура"), scenario, scene.background_texture);

					if (advanced_scenes)
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Background overlay texture", L"Наложенная фоновая текстура"), scenario, scene.background_overlay_texture);
					}

					ImGui::NextColumn();
					sound_selector(LANG_W(L"Background music", L"Фоновая музыка"), scenario, scene.background_music);

					ImGui::NextColumn();
					sound_selector(LANG_W(L"Additional sound", L"Дополнительный звук"), scenario, scene.additional_scene_sound);

					if (advanced_scenes)
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Screen overlay texture", L"Наложенная текстура"), scenario, scene.overlay_texture);
					}

					ImGui::Columns(1);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(LANG(L"Character 1", L"Персонаж 1")))
				{
					ScenarioDialogueScenePersonData_t& person = scene.person1;

					if (advanced_scenes)
						ImGui::Columns(4);
					else
						ImGui::Columns(2);

					std::string dialogue_name = ws2s(person.person_name);

					if (dialogue_name == "NONE")
						dialogue_name = "";

					ImGui::Text(LANG(L"Character name", L"Имя персонажа"));

					ImGui::SetNextItemWidth(200);
					if (ImGui::InputText("##CHARACTER1DIALOGUENAME", &dialogue_name))
						person.person_name = s2ws(dialogue_name);

					std::string dialogue_text = ws2s(person.talking_text);

					if (dialogue_text == "NONE")
						dialogue_text = "";

					ImGui::Text(LANG(L"Character text", L"Текст персонажа"));

					ImGui::SetNextItemWidth(200);
					if (ImGui::InputText("##CHARACTER1DIALOGUETEXT", &dialogue_text))
						person.talking_text = s2ws(dialogue_text);

					if (person.person_name == L"")
						person.person_name = L"NONE";

					if (person.talking_text == L"")
						person.talking_text = L"NONE";

					if (person.talking_text != L"NONE")
						person.talking = true;

					if (advanced_scenes)
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Left body texture", L"Текстура левой части тела"), scenario, person.person_texture_left);

						ImGui::NextColumn();
						texture_selector(LANG_W(L"Right body texture", L"Текстура правой части тела"), scenario, person.person_texture_right);

						ImGui::NextColumn();
						texture_selector(LANG_W(L"Head texture", L"Текстура головы"), scenario, person.person_texture_head);
					}
					else
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Body texture", L"Текстура тела"), scenario, person.person_texture);
					}

					ImGui::Columns(1);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(LANG(L"Character 2", L"Персонаж 2")))
				{
					ScenarioDialogueScenePersonData_t& person = scene.person2;

					if (advanced_scenes)
						ImGui::Columns(4);
					else
						ImGui::Columns(2);

					std::string dialogue_name = ws2s(person.person_name);

					if (dialogue_name == "NONE")
						dialogue_name = "";

					ImGui::Text(LANG(L"Character name", L"Имя персонажа"));

					ImGui::SetNextItemWidth(200);
					if (ImGui::InputText("##CHARACTER2DIALOGUENAME", &dialogue_name))
						person.person_name = s2ws(dialogue_name);

					std::string dialogue_text = ws2s(person.talking_text);

					if (dialogue_text == "NONE")
						dialogue_text = "";

					ImGui::Text(LANG(L"Character text", L"Текст персонажа"));

					ImGui::SetNextItemWidth(200);
					if (ImGui::InputText("##CHARACTER2DIALOGUETEXT", &dialogue_text))
						person.talking_text = s2ws(dialogue_text);

					if (person.person_name == L"")
						person.person_name = L"NONE";

					if (person.talking_text == L"")
						person.talking_text = L"NONE";

					if (person.talking_text != L"NONE")
						person.talking = true;

					if (advanced_scenes)
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Left body texture", L"Текстура левой части тела"), scenario, person.person_texture_left);

						ImGui::NextColumn();
						texture_selector(LANG_W(L"Right body texture", L"Текстура правой части тела"), scenario, person.person_texture_right);

						ImGui::NextColumn();
						texture_selector(LANG_W(L"Head texture", L"Текстура головы"), scenario, person.person_texture_head);
					}
					else
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Body texture", L"Текстура тела"), scenario, person.person_texture);
					}

					ImGui::Columns(1);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(LANG(L"Character 3", L"Персонаж 3")))
				{
					ScenarioDialogueScenePersonData_t& person = scene.person3;

					if (advanced_scenes)
						ImGui::Columns(4);
					else
						ImGui::Columns(2);

					std::string dialogue_name = ws2s(person.person_name);

					if (dialogue_name == "NONE")
						dialogue_name = "";

					ImGui::Text(LANG(L"Character name", L"Имя персонажа"));

					ImGui::SetNextItemWidth(200);
					if (ImGui::InputText("##CHARACTER3DIALOGUENAME", &dialogue_name))
						person.person_name = s2ws(dialogue_name);

					std::string dialogue_text = ws2s(person.talking_text);

					if (dialogue_text == "NONE")
						dialogue_text = "";

					ImGui::Text(LANG(L"Character text", L"Текст персонажа"));

					ImGui::SetNextItemWidth(200);
					if (ImGui::InputText("##CHARACTER3DIALOGUETEXT", &dialogue_text))
						person.talking_text = s2ws(dialogue_text);

					if (person.person_name == L"")
						person.person_name = L"NONE";

					if (person.talking_text == L"")
						person.talking_text = L"NONE";

					if (person.talking_text != L"NONE")
						person.talking = true;

					if (advanced_scenes)
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Left body texture", L"Текстура левой части тела"), scenario, person.person_texture_left);

						ImGui::NextColumn();
						texture_selector(LANG_W(L"Right body texture", L"Текстура правой части тела"), scenario, person.person_texture_right);

						ImGui::NextColumn();
						texture_selector(LANG_W(L"Head texture", L"Текстура головы"), scenario, person.person_texture_head);
					}
					else
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Body texture", L"Текстура тела"), scenario, person.person_texture);
					}

					ImGui::Columns(1);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(LANG(L"Character 4", L"Персонаж 4")))
				{
					ScenarioDialogueScenePersonData_t& person = scene.person4;

					if (advanced_scenes)
						ImGui::Columns(4);
					else
						ImGui::Columns(2);

					std::string dialogue_name = ws2s(person.person_name);

					if (dialogue_name == "NONE")
						dialogue_name = "";

					ImGui::Text(LANG(L"Character name", L"Имя персонажа"));

					ImGui::SetNextItemWidth(200);
					if (ImGui::InputText("##CHARACTER4DIALOGUENAME", &dialogue_name))
						person.person_name = s2ws(dialogue_name);

					std::string dialogue_text = ws2s(person.talking_text);

					if (dialogue_text == "NONE")
						dialogue_text = "";

					ImGui::Text(LANG(L"Character text", L"Текст персонажа"));

					ImGui::SetNextItemWidth(200);
					if (ImGui::InputText("##CHARACTER4DIALOGUETEXT", &dialogue_text))
						person.talking_text = s2ws(dialogue_text);

					if (person.person_name == L"")
						person.person_name = L"NONE";

					if (person.talking_text == L"")
						person.talking_text = L"NONE";

					if (person.talking_text != L"NONE")
						person.talking = true;

					if (advanced_scenes)
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Left body texture", L"Текстура левой части тела"), scenario, person.person_texture_left);

						ImGui::NextColumn();
						texture_selector(LANG_W(L"Right body texture", L"Текстура правой части тела"), scenario, person.person_texture_right);

						ImGui::NextColumn();
						texture_selector(LANG_W(L"Head texture", L"Текстура головы"), scenario, person.person_texture_head);
					}
					else
					{
						ImGui::NextColumn();
						texture_selector(LANG_W(L"Body texture", L"Текстура тела"), scenario, person.person_texture);
					}

					ImGui::Columns(1);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(LANG(L"Buttons", L"Кнопки")))
				{
					ImGui::Columns(4);

					{
						ScenarioDialogueSceneButton_t& button = scene.button1;

						std::string button_name = ws2s(button.button_name);

						if (button_name == "NONE")
							button_name = "";

						ImGui::Text(LANG(L"Button name", L"Название кнопки"));

						ImGui::SetNextItemWidth(200);

						if (ImGui::InputText("##BUTTON1NAME", &button_name))
							button.button_name = s2ws(button_name);

						if (button_name == "")
							button.button_name = L"NONE";

						scenario_selector(LANG_W(L"Scenario to load", L"Сценарий для загрузки"), button.button_scenario_to_load, 1);
					}
					ImGui::NextColumn();

					{
						ScenarioDialogueSceneButton_t& button = scene.button2;

						std::string button_name = ws2s(button.button_name);

						if (button_name == "NONE")
							button_name = "";

						ImGui::Text(LANG(L"Button name", L"Название кнопки"));

						ImGui::SetNextItemWidth(200);

						if (ImGui::InputText("##BUTTON2NAME", &button_name))
							button.button_name = s2ws(button_name);

						if (button_name == "")
							button.button_name = L"NONE";

						scenario_selector(LANG_W(L"Scenario to load", L"Сценарий для загрузки"), button.button_scenario_to_load, 2);
					}
					ImGui::NextColumn();

					{
						ScenarioDialogueSceneButton_t& button = scene.button3;

						std::string button_name = ws2s(button.button_name);

						if (button_name == "NONE")
							button_name = "";

						ImGui::Text(LANG(L"Button name", L"Название кнопки"));

						ImGui::SetNextItemWidth(200);

						if (ImGui::InputText("##BUTTON3NAME", &button_name))
							button.button_name = s2ws(button_name);

						if (button_name == "")
							button.button_name = L"NONE";

						scenario_selector(LANG_W(L"Scenario to load", L"Сценарий для загрузки"), button.button_scenario_to_load, 3);
					}
					ImGui::NextColumn();

					{
						ScenarioDialogueSceneButton_t& button = scene.button4;

						std::string button_name = ws2s(button.button_name);

						if (button_name == "NONE")
							button_name = "";

						ImGui::Text(LANG(L"Button name", L"Название кнопки"));

						ImGui::SetNextItemWidth(200);

						if (ImGui::InputText("##BUTTON4NAME", &button_name))
							button.button_name = s2ws(button_name);

						if (button_name == "")
							button.button_name = L"NONE";

						scenario_selector(LANG_W(L"Scenario to load", L"Сценарий для загрузки"), button.button_scenario_to_load, 4);
					}

					ImGui::Columns(1);

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImGui::End();
		}
	}

	scene.main_character.person_name = main_character_name;

	ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.background_texture, scenario), ImVec2(0, 0), ImGui::GetIO().DisplaySize);

	if (advanced_scenes && scene.background_overlay_texture != L"NONE")
		ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.background_overlay_texture, scenario), ImVec2(0, 0), ImGui::GetIO().DisplaySize);

	if (scene.background_music != L"NONE")
	{
		if (current_playing_music != scene.background_music)
		{
			stream_music(scene.background_music, music_stream);
			current_playing_music = scene.background_music;
		}
	}
	else
	{
		if (current_playing_music != L"")
		{
			BASS_ChannelStop(music_stream);
			BASS_StreamFree(music_stream);

			current_playing_music = L"";
			paused_music = false;
		}
	}

	if (scene.additional_scene_sound != L"NONE")
	{
		if (!additional_channel_playing)
		{
			stream_music(scene.additional_scene_sound, additional_stream);
			additional_channel_playing = true;
		}
	}
	else
	{
		if (additional_channel_playing)
		{
			BASS_ChannelStop(additional_stream);
			BASS_StreamFree(additional_stream);

			additional_channel_playing = false;
		}
	}

	int characters_count = 0;
	int focused_character = -1;

	if (scene.person1.is_valid_texture() || (advanced_scenes && scene.person1.is_valid_texture_left()))
		characters_count++;

	if (scene.person2.is_valid_texture() || (advanced_scenes && scene.person2.is_valid_texture_left()))
		characters_count++;

	if (scene.person3.is_valid_texture() || (advanced_scenes && scene.person3.is_valid_texture_left()))
		characters_count++;

	if (scene.person4.is_valid_texture() || (advanced_scenes && scene.person4.is_valid_texture_left()))
		characters_count++;

	int character_texture_resolution_x = 400 * float(ImGui::GetIO().DisplaySize.x / 1920.0f);
	int character_texture_resolution_x_adv = 960 * float(ImGui::GetIO().DisplaySize.x / 1920.0f);

	std::wstring talking_text = L"";
	std::wstring talking_name = L"";

	if (scene.person1.talking)
	{
		if (talking_name != L"")
			talking_name += L"&";

		talking_name += scene.person1.person_name;

		if (talking_text == L"")
			talking_text = scene.person1.talking_text;

		if (focused_character == -1)
			focused_character = 1;
	}

	if (scene.person2.talking)
	{
		if (talking_name != L"")
			talking_name += L"&";

		talking_name += scene.person2.person_name;

		if (talking_text == L"")
			talking_text = scene.person2.talking_text;

		if (focused_character == -1)
			focused_character = 2;
	}

	if (scene.person3.talking)
	{
		if (talking_name != L"")
			talking_name += L"&";

		talking_name += scene.person3.person_name;

		if (talking_text == L"")
			talking_text = scene.person3.talking_text;

		if (focused_character == -1)
			focused_character = 3;
	}

	if (scene.person4.talking)
	{
		if (talking_name != L"")
			talking_name += L"&";

		talking_name += scene.person4.person_name;

		if (talking_text == L"")
			talking_text = scene.person4.talking_text;

		if (focused_character == -1)
			focused_character = 4;
	}

	if (scene.main_character.talking)
	{
		if (talking_name != L"")
			talking_name += L"&";

		talking_name += scene.main_character.person_name;

		if (talking_text == L"")
			talking_text = scene.main_character.talking_text;

		focused_character = -1;
	}

	int drawn_characters = 0;
	ImVec2 next_position = ImVec2(((ImGui::GetIO().DisplaySize.x - ((character_texture_resolution_x_adv - (character_texture_resolution_x_adv / (characters_count < 4 ? 3 : 2.4f))) * characters_count))) / 2 - (character_texture_resolution_x_adv / (characters_count < 4 ? 5 : 8)), 120 * float(ImGui::GetIO().DisplaySize.y / 1080.0f));

	// characters rendering
	if (scene.person1.is_valid_texture() || (advanced_scenes && scene.person1.is_valid_texture_left()))
	{
		if (advanced_scenes && scene.person1.is_valid_texture_left())
		{
			ImVec2 position = next_position;
			ImVec2 size = ImVec2(character_texture_resolution_x_adv, ImGui::GetIO().DisplaySize.y - (120 * float(ImGui::GetIO().DisplaySize.y / 1080.0f)));

			ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person1.person_texture_left, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			if (scene.person1.is_valid_texture_right())
				ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person1.person_texture_right, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			if (scene.person1.is_valid_texture_head())
				ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person1.person_texture_head, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			next_position = ImVec2(position.x + (character_texture_resolution_x_adv - (character_texture_resolution_x_adv / (characters_count < 4 ? 3 : 2))), position.y);
		}
		else
		{
			ImVec2 position = ImVec2((ImGui::GetIO().DisplaySize.x / 2 - character_texture_resolution_x / 2), 170 * float(ImGui::GetIO().DisplaySize.y / 1080.0f));

			if (characters_count == 2)
				position.x = (ImGui::GetIO().DisplaySize.x / 2 - character_texture_resolution_x + (character_texture_resolution_x * drawn_characters));
			else if (characters_count == 3)
				position.x = ((ImGui::GetIO().DisplaySize.x / 2) - (character_texture_resolution_x + (character_texture_resolution_x / 2)) + (drawn_characters * character_texture_resolution_x));
			else if (characters_count == 4)
				position.x = (((ImGui::GetIO().DisplaySize.x / 2) - (character_texture_resolution_x + character_texture_resolution_x)) + (drawn_characters * character_texture_resolution_x));

			ImVec2 size = ImVec2(character_texture_resolution_x + 50, ImGui::GetIO().DisplaySize.y - (170 * float(ImGui::GetIO().DisplaySize.y / 1080.0f)));

			ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person1.person_texture, scenario), position, ImVec2(position.x + size.x, position.y + size.y));
		}

		drawn_characters++;
	}

	if (scene.person2.is_valid_texture() || (advanced_scenes && scene.person2.is_valid_texture_left()))
	{
		if (advanced_scenes && scene.person2.is_valid_texture_left())
		{
			ImVec2 position = next_position;
			ImVec2 size = ImVec2(character_texture_resolution_x_adv, ImGui::GetIO().DisplaySize.y - (120 * float(ImGui::GetIO().DisplaySize.y / 1080.0f)));

			ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person2.person_texture_left, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			if (scene.person2.is_valid_texture_right())
				ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person2.person_texture_right, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			if (scene.person2.is_valid_texture_head())
				ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person2.person_texture_head, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			next_position = ImVec2(position.x + (character_texture_resolution_x_adv - (character_texture_resolution_x_adv / (characters_count < 4 ? 3 : 2))), position.y);
		}
		else
		{
			ImVec2 position = ImVec2((ImGui::GetIO().DisplaySize.x / 2 - character_texture_resolution_x / 2), 170 * float(ImGui::GetIO().DisplaySize.y / 1080.0f));

			if (characters_count == 2)
				position.x = (ImGui::GetIO().DisplaySize.x / 2 - character_texture_resolution_x + (character_texture_resolution_x * drawn_characters));
			else if (characters_count == 3)
				position.x = ((ImGui::GetIO().DisplaySize.x / 2) - (character_texture_resolution_x + (character_texture_resolution_x / 2)) + (drawn_characters * character_texture_resolution_x));
			else if (characters_count == 4)
				position.x = (((ImGui::GetIO().DisplaySize.x / 2) - (character_texture_resolution_x + character_texture_resolution_x)) + (drawn_characters * character_texture_resolution_x));

			ImVec2 size = ImVec2(character_texture_resolution_x + 50, ImGui::GetIO().DisplaySize.y - (170 * float(ImGui::GetIO().DisplaySize.y / 1080.0f)));

			ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person2.person_texture, scenario), position, ImVec2(position.x + size.x, position.y + size.y));
		}

		drawn_characters++;
	}

	if (scene.person3.is_valid_texture() || (advanced_scenes && scene.person3.is_valid_texture_left()))
	{
		if (advanced_scenes && scene.person3.is_valid_texture_left())
		{
			ImVec2 position = next_position;
			ImVec2 size = ImVec2(character_texture_resolution_x_adv, ImGui::GetIO().DisplaySize.y - (120 * float(ImGui::GetIO().DisplaySize.y / 1080.0f)));

			ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person3.person_texture_left, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			if (scene.person3.is_valid_texture_right())
				ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person3.person_texture_right, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			if (scene.person3.is_valid_texture_head())
				ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person3.person_texture_head, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			next_position = ImVec2(position.x + (character_texture_resolution_x_adv - (character_texture_resolution_x_adv / (characters_count < 4 ? 3 : 2))), position.y);
		}
		else
		{
			ImVec2 position = ImVec2((ImGui::GetIO().DisplaySize.x / 2 - character_texture_resolution_x / 2), 170 * float(ImGui::GetIO().DisplaySize.y / 1080.0f));

			if (characters_count == 2)
				position.x = (ImGui::GetIO().DisplaySize.x / 2 - character_texture_resolution_x + (character_texture_resolution_x * drawn_characters));
			else if (characters_count == 3)
				position.x = ((ImGui::GetIO().DisplaySize.x / 2) - (character_texture_resolution_x + (character_texture_resolution_x / 2)) + (drawn_characters * character_texture_resolution_x));
			else if (characters_count == 4)
				position.x = (((ImGui::GetIO().DisplaySize.x / 2) - (character_texture_resolution_x + character_texture_resolution_x)) + (drawn_characters * character_texture_resolution_x));

			ImVec2 size = ImVec2(character_texture_resolution_x + 50, ImGui::GetIO().DisplaySize.y - (170 * float(ImGui::GetIO().DisplaySize.y / 1080.0f)));

			ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person3.person_texture, scenario), position, ImVec2(position.x + size.x, position.y + size.y));
		}

		drawn_characters++;
	}

	if (scene.person4.is_valid_texture() || (advanced_scenes && scene.person4.is_valid_texture_left()))
	{
		if (advanced_scenes && scene.person4.is_valid_texture_left())
		{
			ImVec2 position = next_position;
			ImVec2 size = ImVec2(character_texture_resolution_x_adv, ImGui::GetIO().DisplaySize.y - (120 * float(ImGui::GetIO().DisplaySize.y / 1080.0f)));

			ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person4.person_texture_left, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			if (scene.person4.is_valid_texture_right())
				ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person4.person_texture_right, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			if (scene.person4.is_valid_texture_head())
				ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person4.person_texture_head, scenario), position, ImVec2(position.x + size.x, position.y + size.y));

			next_position = ImVec2(position.x + (character_texture_resolution_x_adv - (character_texture_resolution_x_adv / (characters_count < 4 ? 3 : 2))), position.y);
		}
		else
		{
			ImVec2 position = ImVec2((ImGui::GetIO().DisplaySize.x / 2 - character_texture_resolution_x / 2), 170 * float(ImGui::GetIO().DisplaySize.y / 1080.0f));

			if (characters_count == 2)
				position.x = (ImGui::GetIO().DisplaySize.x / 2 - character_texture_resolution_x + (character_texture_resolution_x * drawn_characters));
			else if (characters_count == 3)
				position.x = ((ImGui::GetIO().DisplaySize.x / 2) - (character_texture_resolution_x + (character_texture_resolution_x / 2)) + (drawn_characters * character_texture_resolution_x));
			else if (characters_count == 4)
				position.x = (((ImGui::GetIO().DisplaySize.x / 2) - (character_texture_resolution_x + character_texture_resolution_x)) + (drawn_characters * character_texture_resolution_x));

			ImVec2 size = ImVec2(character_texture_resolution_x + 50, ImGui::GetIO().DisplaySize.y - (170 * float(ImGui::GetIO().DisplaySize.y / 1080.0f)));

			ImGui::GetBackgroundDrawList()->AddImage(find_texture(scene.person4.person_texture, scenario), position, ImVec2(position.x + size.x, position.y + size.y));
		}

		drawn_characters++;
	}

	ImVec2 position = ImVec2(200, (ImGui::GetIO().DisplaySize.y - ImGui::GetIO().DisplaySize.y / 4.0f));
	ImVec2 size = ImVec2(ImGui::GetIO().DisplaySize.x - 400, ImGui::GetIO().DisplaySize.y / 4.0f - 50);

	size.y = ImGui::CalcTextSize(utf8(talking_name.c_str())).y * 6 + 55;
	position.y = ImGui::GetIO().DisplaySize.y - 20 - size.y;

	if (talking_name == scene.main_character.person_name && talking_text.at(scene.main_character.talking_text.length() - 1) == L'@')
	{
		talking_text.erase(scene.main_character.talking_text.length() - 1);
		talking_name = L"";
	}

	if (talking_text.find(L"@MAINCHARACTERNAME") != std::wstring::npos)
	{
		std::vector<std::wstring> splitted_str = split_string(talking_text, L'@');
		talking_text = splitted_str.at(0) + main_character_name + splitted_str.at(1).substr(17);
	}

	if (talking_name != L"")
	{
		ImGui::PushFont(game_fonts.dialogue_name_font.font_data);

		ImGui::SetNextWindowPos(ImVec2(position.x + 30, position.y - ImGui::CalcTextSize(utf8(talking_name.c_str())).y - 20));
		ImGui::SetNextWindowSize(ImVec2(ImGui::CalcTextSize(utf8(talking_name.c_str())).x + 20, ImGui::CalcTextSize(utf8(talking_name.c_str())).y + 22));

		ImGui::Begin("##DIALOGUENAMEWINDOW", (bool*)0, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground);
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(position.x + 30, position.y - ImGui::CalcTextSize(utf8(talking_name.c_str())).y - 20), ImVec2(position.x + 30 + ImGui::CalcTextSize(utf8(talking_name.c_str())).x + 20, position.y + 2), ImColor(20, 20, 20, 135));

		ImGui::Text(utf8(talking_name.c_str()));

		ImGui::End();

		ImGui::PopFont();
	}

	ImGui::SetNextWindowPos(position);
	ImGui::SetNextWindowSize(size);

	ImGui::Begin("##DIALOGUETEXTWINDOW", (bool*)0, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground);
	ImGui::GetWindowDrawList()->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), ImColor(20, 20, 20, 135));

	// text animation
	if (game_settings.animated_dialogue_text)
	{
		if (!game_menu_open)
			dialogue_text_animation_lerp = ImLerp(dialogue_text_animation_lerp, 1.0f, ImGui::GetIO().DeltaTime * float(float(game_settings.text_animation_speed) / 100.0f));

		if (dialogue_added_text_symbols < talking_text.length() && dialogue_text_animation_lerp > 0.01f)
		{
			dialogue_text_to_render += talking_text.at(dialogue_added_text_symbols);
			dialogue_added_text_symbols++;
		}

		if (dialogue_text_animation_lerp > 0.01f)
			dialogue_text_animation_lerp = 0.0f;
	}
	else
		dialogue_text_to_render = talking_text;

	ImGui::PushFont(game_fonts.dialogue_text_font.font_data);
	ImGui::TextWrapped(utf8(dialogue_text_to_render.c_str()));
	ImGui::PopFont();

	ImGui::End();

	if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
	{
		game_menu_open = !game_menu_open;

		if (!game_menu_open)
			save_menu_open = false;
	}

	if (scene.button1.is_present() || scene.button2.is_present() || scene.button3.is_present() || scene.button4.is_present())
		button_menu_open = true;
	else
		button_menu_open = false;

	bool clicked_button = false;

	if (button_menu_open)
	{
		disable_input_on_scene = true;

		ImVec2 size = ImVec2(250, 120);
		ImVec2 position = ImVec2(ImGui::GetIO().DisplaySize.x / 2 - size.x / 2, ImGui::GetIO().DisplaySize.y / 2 - size.y / 2);

		ImGui::SetNextWindowSize(size);
		ImGui::SetNextWindowPos(position);
		ImGui::Begin("##GAMEBUTTONMENU", &save_menu_open, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

		if (scene.button1.is_present())
		{
			if (ImGui::Button(utf8(scene.button1.button_name.c_str()), ImVec2(230, 20)))
			{
				button_menu_open = false;
				clicked_button = true;

				if (scene.button1.has_scenario_action())
				{
					int scenario_idx = find_scenario_index(scene.button1.button_scenario_to_load);

					if (scenario_idx != -1)
					{
						exit_to_main_menu(true);

						game_started = true;

						selected_scenario = scenario_idx;
						load_scenario_data(selected_scenario);

						return;
					}
				}
			}
		}

		if (scene.button2.is_present())
		{
			if (ImGui::Button(utf8(scene.button2.button_name.c_str()), ImVec2(230, 20)))
			{
				button_menu_open = false;
				clicked_button = true;

				if (scene.button2.has_scenario_action())
				{
					int scenario_idx = find_scenario_index(scene.button2.button_scenario_to_load);

					if (scenario_idx != -1)
					{
						exit_to_main_menu(true);

						game_started = true;

						selected_scenario = scenario_idx;
						load_scenario_data(selected_scenario);

						return;
					}
				}
			}
		}

		if (scene.button3.is_present())
		{
			if (ImGui::Button(utf8(scene.button3.button_name.c_str()), ImVec2(230, 20)))
			{
				button_menu_open = false;
				clicked_button = true;

				if (scene.button3.has_scenario_action())
				{
					int scenario_idx = find_scenario_index(scene.button3.button_scenario_to_load);

					if (scenario_idx != -1)
					{
						exit_to_main_menu(true);

						game_started = true;

						selected_scenario = scenario_idx;
						load_scenario_data(selected_scenario);

						return;
					}
				}
			}
		}

		if (scene.button4.is_present())
		{
			if (ImGui::Button(utf8(scene.button4.button_name.c_str()), ImVec2(230, 20)))
			{
				if (scene.button4.has_scenario_action())
				{
					button_menu_open = false;
					clicked_button = true;

					if (scene.button4.has_scenario_action())
					{
						int scenario_idx = find_scenario_index(scene.button4.button_scenario_to_load);

						if (scenario_idx != -1)
						{
							exit_to_main_menu(true);

							game_started = true;

							selected_scenario = scenario_idx;
							load_scenario_data(selected_scenario);

							return;
						}
					}
				}
			}
		}

		ImGui::End();
	}

	if (advanced_scenes && scene.overlay_texture != L"NONE")
	{
#ifndef DCS_OPENGL
		ID3D11ShaderResourceView* texture = find_texture(scene.overlay_texture, scenario);

		ID3D11Resource* src;
		texture->GetResource(&src);

		ID3D11Texture2D* t2d;
		src->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&t2d));

		D3D11_TEXTURE2D_DESC desc;
		t2d->GetDesc(&desc);

		ImGui::GetBackgroundDrawList()->AddImage(texture, ImVec2(ImGui::GetIO().DisplaySize.x / 2 - desc.Width / 2, ImGui::GetIO().DisplaySize.y / 2 - desc.Height / 2), ImVec2(ImGui::GetIO().DisplaySize.x / 2 + desc.Width / 2, ImGui::GetIO().DisplaySize.y / 2 + desc.Height / 2));
#else
		sf::Texture& texture = find_texture_sf(scene.overlay_texture, scenario);
		ImGui::GetBackgroundDrawList()->AddImage(convertGLtoImTexture(texture.getNativeHandle()), ImVec2(ImGui::GetIO().DisplaySize.x / 2 - texture.getSize().x / 2, ImGui::GetIO().DisplaySize.y / 2 - texture.getSize().y / 2), ImVec2(ImGui::GetIO().DisplaySize.x / 2 + texture.getSize().x / 2, ImGui::GetIO().DisplaySize.y / 2 + texture.getSize().y / 2));
#endif
	}

	if (talking_name != L"" && talking_text != L"" && !recorded_dialogue)
	{
		dialogue_history.push_back(std::wstring(talking_name + L": " + talking_text));
		recorded_dialogue = true;
	}

	if (game_menu_open)
	{
		disable_input_on_scene = true;
		button_menu_open = false;

		if (!paused_music && current_playing_music != L"")
		{
			BASS_ChannelPause(music_stream);
			paused_music = true;
		}

		main_game_menu(scenario);
	}
	else
	{
		video_settings_open = false;
		audio_settings_open = false;
		game_settings_open = false;

		if (paused_music && current_playing_music != L"")
		{
			BASS_ChannelPlay(music_stream, FALSE);
			paused_music = false;
		}

		if (!button_menu_open)
			disable_input_on_scene = false;
	}

	if (!scenario_editor)
	{
		if (((!disable_input_on_scene && GetForegroundWindow() == hWnd) || clicked_button) && (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space), false) || ImGui::IsMouseClicked(0) || clicked_button) && (current_scenario_scene + 1) < scenario.scenes.size())
		{
			BASS_ChannelStop(additional_stream);
			BASS_StreamFree(additional_stream);

			additional_channel_playing = false;
			recorded_dialogue = false;

			dialogue_text_to_render = L"";
			dialogue_added_text_symbols = 0;

			dialogue_text_animation_lerp = 0.0f;

			current_scenario_scene++;
		}
	}
}

void game_render()
{
	ImGui::PushFont(game_fonts.main_menu_font.font_data);

	float music_volume = float(float(audio_settings.music_volume) / float(100.0f));
	BASS_ChannelSetAttribute(music_stream, BASS_ATTRIB_VOL, music_volume);

	float sound_volume = float(float(audio_settings.sound_volume) / float(100.0f));
	BASS_ChannelSetAttribute(additional_stream, BASS_ATTRIB_VOL, sound_volume);

	if (paused_music && current_playing_music == L"")
		paused_music = false;

	if (game_settings.show_fps_counter)
	{
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 105, 10));
		ImGui::SetNextWindowSize(ImVec2(95, 35));
		ImGui::Begin("##FPSCOUNTER", (bool*)0, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground);

		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(ImGui::GetIO().DisplaySize.x - 105, 10), ImVec2(ImGui::GetIO().DisplaySize.x - 10, 45), ImColor(10, 10, 10, 190));
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		ImGui::End();
	}

	if (!game_started)
	{
		if (current_playing_music != L"menu_background")
		{
			stream_music(L"menu_background", music_stream);
			current_playing_music = L"menu_background";
		}

		if (additional_channel_playing)
		{
			BASS_ChannelStop(additional_stream);
			BASS_StreamFree(additional_stream);

			additional_channel_playing = false;
		}

#ifndef DCS_OPENGL
		ImGui::GetBackgroundDrawList()->AddImage(game_menu_data.main_menu_background.image_data, ImVec2(0, 0), ImGui::GetIO().DisplaySize);
#else
		ImGui::GetBackgroundDrawList()->AddImage(convertGLtoImTexture(game_menu_data.main_menu_background.image_data.getNativeHandle()), ImVec2(0, 0), ImGui::GetIO().DisplaySize);
#endif

		ImVec2 window_size = ImVec2(250, ImGui::GetIO().DisplaySize.y - 40);
		ImVec2 window_pos = ImVec2(20, 20);

		ImGui::SetNextWindowSize(window_size);
		ImGui::SetNextWindowPos(window_pos);
		ImGui::Begin("##MAINMENU", (bool*)0, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

		settings_render_position = ImVec2(window_pos.x + window_size.x + 10, window_pos.y);

		ImGui::Image(game_menu_data.game_logo.image_data, ImVec2(230, 120));

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (select_scenario)
		{
			if (ImGui::Button("<", ImVec2(230, 20)))
				select_scenario = false;

#ifndef DCS_STORY_GAME
			ImGui::Text(LANG(L"Select scenario", L"Выберите сценарий"));

			ImGui::SetNextItemWidth(230);
			if (ImGui::BeginListBox("##Scenario selector"))
			{
				for (int i = 0; i < scenario_names.size(); i++)
				{
					if (ImGui::Selectable(scenario_names.at(i).c_str(), selected_scenario == i))
						selected_scenario = i;
				}

				ImGui::EndListBox();
			}
#endif

#ifdef DCS_STORY_GAME
			selected_scenario = find_scenario_index(L"main.sc");
#endif

			if (scenario_editor)
			{
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				static char scenario_name[64];
				ImGui::Text(LANG(L"Enter new scenario name", L"Введите название нового сценария"));

				ImGui::SetNextItemWidth(230);
				ImGui::InputText("##Scenario name input", scenario_name, 64);

				if (ImGui::Button(LANG(L"Create new scenario", L"Создать новый сценарий"), ImVec2(230, 20)))
				{
					create_new_scenario(s2ws(std::string(scenario_name)));

					scenarios = load_scenarios(L".\\game\\scenarios", L".\\game\\textures", L".\\game\\sounds");
					scenario_names = get_directory_files_name(".\\game\\scenarios", ".sc");
				}
			}

			if (selected_scenario > -1)
			{
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				static char player_name[32];
				ImGui::Text(LANG(L"Enter player name", L"Введите имя игрока"));

				ImGui::SetNextItemWidth(230);
				ImGui::InputText("##Player name input", player_name, 32);

				if (std::string(player_name) != "" && std::string(player_name) != " ")
				{
					if (ImGui::Button(scenario_editor ? LANG(L"Edit", L"Редактировать") : LANG(L"Play", L"Играть"), ImVec2(230, 20)))
					{
						wchar_t WBuf[32];
						mbstowcs(WBuf, player_name, 31);

						main_character_name = std::wstring(WBuf);

						game_started = true;

						video_settings_open = false;
						audio_settings_open = false;
						game_settings_open = false;
					}
				}
			}
		}
		else if (select_save)
		{
			if (ImGui::Button("<", ImVec2(230, 20)))
				select_save = false;

			ImGui::Text(LANG(L"Select save", L"Выберите сохранение"));

			ImGui::SetNextItemWidth(230);
			if (ImGui::BeginListBox("##Scenario save"))
			{
				for (int i = 0; i < save_names.size(); i++)
				{
					if (ImGui::Selectable(save_names.at(i).c_str(), selected_save == i))
						selected_save = i;
				}

				ImGui::EndListBox();
			}

			if (ImGui::Button(LANG(L"Refresh list", L"Обновить список"), ImVec2(230, 20)))
			{
				saves = get_directory_files_name_w(L".\\game\\saves", L".savegame");
				save_names = get_directory_files_name(".\\game\\saves", ".savegame");
			}

			if (selected_save > -1)
			{
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (ImGui::Button(LANG(L"Play", L"Играть"), ImVec2(230, 20)))
				{
					GameSave_t save;

					if (load_save(saves.at(selected_save), save) && find_scenario_index(save.scenario_name) != -1)
					{
						main_character_name = save.player_name;

						selected_scenario = find_scenario_index(save.scenario_name);
						current_scenario_scene = save.scenario_scene;

						game_started = true;

						video_settings_open = false;
						audio_settings_open = false;
						game_settings_open = false;
					}
					else
						MessageBoxW(GetForegroundWindow(), LANG_W(L"Failed to load save", L"Не удалось загрузить сохранение"), LANG_W(L"Error", L"Ошибка"), 0);
				}
			}
		}
		else
		{
			if (ImGui::Button(scenario_editor ? LANG(L"Edit scenario", L"Редактировать сценарий") : LANG(L"Start new game", L"Начать новую игру"), ImVec2(230, 20)))
				select_scenario = true;

			if (!scenario_editor)
			{
				if (ImGui::Button(LANG(L"Load save", L"Загрузить сохранение"), ImVec2(230, 20)))
					select_save = true;
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button(LANG(L"Video settings", L"Настройки видео"), ImVec2(230, 20)))
				video_settings_open = !video_settings_open;

			if (ImGui::Button(LANG(L"Audio settings", L"Настройки звука"), ImVec2(230, 20)))
				audio_settings_open = !audio_settings_open;

			if (ImGui::Button(LANG(L"Game settings", L"Настройки игры"), ImVec2(230, 20)))
				game_settings_open = !game_settings_open;

			if (wcsstr(GetCommandLineW(), L"-skid1337"))
			{
				if (!add_start_scene && ImGui::Button("Click me!", ImVec2(230, 20)))
					add_start_scene = true;
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button(LANG(L"Quit game", L"Выход из игры"), ImVec2(230, 20)))
				exit(0);
		}

		ImGui::End();

		button_menu_open = false;
	}
	else
	{
		select_scenario = false;
		select_save = false;

		// load scenario if it wasnt loaded during game init
		load_scenario_data(selected_scenario);

		main_game();
	}

	if (video_settings_open)
		video_settings_menu();

	if (audio_settings_open)
		audio_settings_menu();

	if (game_settings_open)
		game_settings_menu();
}

bool intro_render()
{
	static float delta_time_factor = 1.0f;

	if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space)))
		delta_time_factor = 8.0f;

	if (!rendered_intro)
	{
		intro_alpha_fade_out = ImLerp(intro_alpha_fade_out, 3.0f, ImGui::GetIO().DeltaTime * (delta_time_factor - 0.6f));

		if (intro_alpha_fade_out > 2.60f)
			intro_alpha_fade_in = ImLerp(intro_alpha_fade_in, 3.0f, ImGui::GetIO().DeltaTime * delta_time_factor);

#ifndef DCS_OPENGL
		ImGui::GetForegroundDrawList()->AddImage(game_menu_data.intro_logo.image_data, ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 150, ImGui::GetIO().DisplaySize.y / 2 - 180), ImVec2(ImGui::GetIO().DisplaySize.x / 2 + 150, ImGui::GetIO().DisplaySize.y / 2 + 120), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, int(255 * ImClamp(intro_alpha_fade_out - intro_alpha_fade_in, 0.0f, 1.0f))));
#else
		ImGui::GetForegroundDrawList()->AddImage(convertGLtoImTexture(game_menu_data.intro_logo.image_data.getNativeHandle()), ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 150, ImGui::GetIO().DisplaySize.y / 2 - 180), ImVec2(ImGui::GetIO().DisplaySize.x / 2 + 150, ImGui::GetIO().DisplaySize.y / 2 + 120), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, int(255 * ImClamp(intro_alpha_fade_out - intro_alpha_fade_in, 0.0f, 1.0f))));
#endif
		ImGui::GetForegroundDrawList()->AddText(game_fonts.intro_font.font_data, game_fonts.intro_font.font_size, ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 150, ImGui::GetIO().DisplaySize.y / 2 + 120), ImColor(255, 255, 255, int(255 * ImClamp(intro_alpha_fade_out - intro_alpha_fade_in, 0.0f, 1.0f))), utf8(game_info.game_developer.c_str()));

		if (intro_alpha_fade_in > 2.8f)
			rendered_intro = true;
	}

	return rendered_intro;
}

#define DCS_DEBUG

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nShowCmd)
{
	if (wcsstr(GetCommandLineW(), L"-scenario_editor"))
		scenario_editor = true;

	video_settings.screen_width = 1280;
	video_settings.screen_height = 720;

	hInstance = hInst;
	
#ifndef DCS_OPENGL
	hookKeyboardProc(hInstance);
#endif

#ifdef DCS_DEBUG
	AllocConsole();

	FILE* out;
	freopen_s(&out, "CONOUT$", "w", stdout);

	FILE* in;
	freopen_s(&in, "CONIN$", "r", stdin);
#endif

	read_game_info_from_file();
	write_game_info_to_file(game_info);

	load_language_files();

#ifndef DCS_STORY_GAME
	if (wcsstr(GetCommandLineW(), L"-advanced_scenes"))
		advanced_scenes = true;
#endif

#ifdef DISCORD_RPC
	if (!wcsstr(GetCommandLineW(), L"-disable_discord_rpc"))
	{
		RPC_Initialize(utf8(game_info.game_rpc_app_id.c_str()));

		DiscordRichPresence discordPresence;
		memset(&discordPresence, 0, sizeof(discordPresence));

		char image_key[260];
		wcstombs(image_key, game_info.game_rpc_app_logo.c_str(), 259);

		discordPresence.largeImageKey = image_key;
		discordPresence.startTimestamp = discord_start_timestamp = std::time(0);

		RPC_UpdatePresence(discordPresence);

#ifndef DCS_STORY_GAME
		CreateThread(0, 0, rpc_update_thread, 0, 0, 0);
#endif
	}
#endif

	CreateDirectoryW(L".\\game\\", NULL);
	CreateDirectoryW(L".\\game\\textures\\", NULL);
	CreateDirectoryW(L".\\game\\images\\", NULL);
	CreateDirectoryW(L".\\game\\images\\menu\\", NULL);
	CreateDirectoryW(L".\\game\\scenarios\\", NULL);
	CreateDirectoryW(L".\\game\\translations\\", NULL);
	CreateDirectoryW(L".\\game\\screenshots\\", NULL);
	CreateDirectoryW(L".\\game\\saves\\", NULL);
	CreateDirectoryW(L".\\game\\sounds\\", NULL);
	CreateDirectoryW(L".\\game\\fonts\\", NULL);
	CreateDirectoryW(L".\\game\\config\\", NULL);

	scenarios = load_scenarios(L".\\game\\scenarios", L".\\game\\textures", L".\\game\\sounds");
	scenario_names = get_directory_files_name(".\\game\\scenarios", ".sc");

	saves = get_directory_files_name_w(L".\\game\\saves", L".savegame");
	save_names = get_directory_files_name(".\\game\\saves", ".savegame");

	read_audio_settings_from_file();
	write_audio_settings_to_file(audio_settings);

	read_video_settings_from_file();
	write_video_settings_to_file(video_settings);

	read_game_settings_from_file();
	write_game_settings_to_file(game_settings);

	read_game_fonts_from_file();
	write_game_fonts_to_file(game_fonts);

	BASS_Init(-1, 44100, 0, 0, NULL);

#ifdef DCS_OPENGL
	if (video_settings.screen_mode == 0)
	{
		video_settings.screen_width = 1920;
		video_settings.screen_height = 1080;
	}
#endif

#ifndef DCS_OPENGL
RENDER_INIT:
	WNDCLASSEX wcx = { 0 };
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = wndProc;
	wcx.hInstance = hInstance;
	wcx.lpszClassName = L"KOMOKSOFTWARE";

	if (!RegisterClassEx(&wcx))
		return 1;

	hWnd = CreateWindowExW(0,
		L"KOMOKSOFTWARE",
		std::wstring(game_info.game_name + L" (" + game_renderer + L")").c_str(),
		WS_OVERLAPPEDWINDOW,
		300, 300,
		video_settings.screen_width + 15, video_settings.screen_height + 60,
		0,
		0,
		hInstance,
		0);

	if (!hWnd)
		return 2;

	ShowWindow(hWnd, nShowCmd);

	if (!initDirectX())
		return 2;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
#else
	sf::RenderWindow window(sf::VideoMode(video_settings.screen_width, video_settings.screen_height), std::wstring(game_info.game_name + L" (" + game_renderer + L")").c_str(), sf::Style::Titlebar | sf::Style::Close | (video_settings.screen_mode == 0 ? sf::Style::Fullscreen : 0));

	hWnd = window.getSystemHandle();
	window.setVerticalSyncEnabled(video_settings.vsync);

	ImGui::SFML::Init(window);
#endif

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;

	ImGui::StyleColorsDCS_Dark();

	ImGui::GetStyle().WindowPadding = ImVec2(10, 10);
	ImGui::GetStyle().WindowRounding = 2.0f;

	ImGui::GetStyle().TabRounding = 2.0f;

	if (!first_init)
	{
		load_game_assets(io);
		first_init = true;
	}
	else
	{
#ifndef DCS_OPENGL
		if (should_recreate_d3d_device_and_window)
			reload_game_assets(io);
#endif
	}

#ifndef DCS_OPENGL
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	MSG msg;

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			break;

		//else
		//	SendMessage(hWnd, WM_RENDER, NULL, NULL);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		if (intro_render())
			game_render();

		if (GetAsyncKeyState(VK_F12) & 1)
		{
			HRESULT hr = S_OK;
			ID3D11Resource* pSurface = nullptr;

			hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pSurface));
			if (FAILED(hr) || pSurface == nullptr)
			{

			}

			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Width = ImGui::GetIO().DisplaySize.x;
			desc.Height = ImGui::GetIO().DisplaySize.y;
			desc.MipLevels = 1;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.BindFlags = 0;
			desc.CPUAccessFlags = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;

			ID3D11Texture2D* pTexture = nullptr;

			hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &pTexture);
			if (pTexture)
			{
				g_pd3dDeviceContext->ResolveSubresource(pTexture, 0, pSurface, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

				hr = D3DX11SaveTextureToFileW(g_pd3dDeviceContext, pTexture, D3DX11_IFF_PNG, std::wstring(L".\\game\\screenshots\\" + get_screenshot_name()).c_str());
				pTexture->Release();
			}
			pSurface->Release();
		}

		ImGui::EndFrame();

		ImGui::Render();

		g_pd3dDeviceContext->OMSetRenderTargets(1, &backBuffer, NULL);
		g_pd3dDeviceContext->ClearDepthStencilView(depthStancilBuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

		float color[4] = { 0.03f, 0.03f, 0.03f, 1.0f };
		g_pd3dDeviceContext->ClearRenderTargetView(backBuffer, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present((int)video_settings.vsync, 0);
	}

	if (should_recreate_d3d_device_and_window)
		unload_game_images_and_textures();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	clear();
#else
	sf::Clock deltaClock;
	while (window.isOpen()) 
	{
		sf::Event event;

		while (window.pollEvent(event)) 
		{
			if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F12)
			{
				sf::Image screenshot = window.capture();
				screenshot.saveToFile(ws2s(std::wstring(L".\\game\\screenshots\\" + get_screenshot_name())));
			}

			ImGui::SFML::ProcessEvent(window, event);

			if (event.type == sf::Event::Closed)
				window.close();
		}

		ImGui::SFML::Update(window, deltaClock.restart());

		if (intro_render())
			game_render();

		window.clear();
		ImGui::SFML::Render(window);

		window.display();
	}

	ImGui::SFML::Shutdown();
#endif

	BASS_Free();

	return 0;
}

#ifndef DCS_OPENGL
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		SIZE p;
		p.cx = LOWORD(lParam);
		p.cy = HIWORD(lParam);

		if (resizeWindow(p) == -1)
			PostMessage(hWnd, WM_QUIT, NULL, NULL);

		break;
	case WM_ACTIVATE:
		if (video_settings.screen_mode == 0 && IsIconic(hWnd) && WA_ACTIVE == LOWORD(wParam))
		{
			isRender = true;
			SetWindowPlacement(hWnd, &fullscreenPlacement);
			g_pSwapChain->SetFullscreenState(true, nullptr);
		}
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool initDirectX()
{
	// init dx
	DXGI_SWAP_CHAIN_DESC sdesc;
	ZeroMemory(&sdesc, sizeof(sdesc));
	sdesc.BufferCount = 1;
	sdesc.OutputWindow = hWnd;
	sdesc.SampleDesc.Count = 1;
	sdesc.SampleDesc.Quality = 0;
	sdesc.Windowed = true;
	sdesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sdesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sdesc.BufferDesc.Height = video_settings.screen_height;
	sdesc.BufferDesc.Width = video_settings.screen_width;

	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	UINT flags(NULL);

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE,
		NULL, flags, levels, 6, D3D11_SDK_VERSION, &sdesc,
		&g_pSwapChain, &g_pd3dDevice, NULL, &g_pd3dDeviceContext);
	if (hr != S_OK)
		return false;

	if (!enabelAltEnter) // disable mode alt + enter
	{
		IDXGIDevice* pDXGIDevice;
		HRESULT hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
		if (hr != S_OK)
			return false;

		IDXGIAdapter* pDXGIAdapter;
		hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter);
		if (hr != S_OK)
			return false;

		IDXGIFactory* pIDXGIFactory;
		hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pIDXGIFactory);
		if (hr != S_OK)
			return false;

		hr = pIDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
		if (hr != S_OK)
			return false;
	}

	isRender = true;

	if (resizeWindow({ video_settings.screen_width, video_settings.screen_height }) == -1)
		return false;
	if (!renderStateEdit(D3D11_FILL_MODE::D3D11_FILL_SOLID))
		return false;

	hr = g_pSwapChain->SetFullscreenState(bool(video_settings.screen_mode == 0), nullptr);
	if (hr != S_OK)
		return false;

	return true;
}

bool hookKeyboardProc(HINSTANCE hinst)
{
	ExistingKeyboardProc = SetWindowsHookEx(
		WH_KEYBOARD_LL,
		keyboardProcLowLevel,
		hinst,
		NULL);

	if (!ExistingKeyboardProc)
		return false;
	else
		return true;
}

LRESULT CALLBACK keyboardProcLowLevel(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* hookstruct = (KBDLLHOOKSTRUCT*)(lParam);

	switch (wParam)
	{
	case WM_KEYDOWN:
		break;
	case WM_SYSKEYDOWN:
		if ((((hookstruct->flags) >> 5) & 1))
		{
			// ALT +
			switch (hookstruct->vkCode)
			{
			case VK_TAB: // ALT+TAB
			{
				if (video_settings.screen_mode == 0 && !IsIconic(hWnd))
				{
					isRender = false;
					fullscreenPlacement.length = sizeof(WINDOWPLACEMENT);
					GetWindowPlacement(hWnd, &fullscreenPlacement);
					ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
					g_pSwapChain->SetFullscreenState(false, nullptr);
				}
			}
			break;
			case VK_RETURN: // ALT+ENTER
				break;
			case VK_ESCAPE: // ALT+ESC
				break;
			case VK_DELETE: // ALT+DEL
				break;
			};
		}
		break;
	case WM_KEYUP:
		break;
	case WM_SYSKEYUP:
		break;
	}

	return CallNextHookEx(ExistingKeyboardProc, nCode, wParam, lParam);
}

int unHookKeyboardProc()
{
	if (ExistingKeyboardProc)
	{
		BOOL retcode = UnhookWindowsHookEx((HHOOK)keyboardProcLowLevel);

		if (retcode)
		{
			// Successfully 
		}
		else
		{
			//Error 
		}
		return retcode;
	}
	else
	{
		//Error 
		return -1;
	}
}

void createDescsTarget()
{
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = video_settings.screen_width;
	descDepth.Height = video_settings.screen_height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 4;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	descDSV.Texture2D.MipSlice = 0;

	vp.Width = (float)video_settings.screen_width;
	vp.Height = (float)video_settings.screen_height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
}

bool createTargetRender()
{
	if (backBuffer)
		backBuffer->Release();

	if (depthStancil)
		depthStancil->Release();

	if (depthStancilBuffer)
		depthStancilBuffer->Release();

	HRESULT hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	if (hr != S_OK)
		return false;

	ID3D11Texture2D* texture(nullptr);
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&texture);
	hr = g_pd3dDevice->CreateRenderTargetView(texture, NULL, &backBuffer);
	if (hr != S_OK)
		return false;
	texture->Release();

	createDescsTarget();

	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &depthStancil);
	if (hr != S_OK)
		return false;

	hr = g_pd3dDevice->CreateDepthStencilView(depthStancil, &descDSV, &depthStancilBuffer);
	if (hr != S_OK)
		return false;

	g_pd3dDeviceContext->OMSetRenderTargets(1, &backBuffer, depthStancilBuffer);
	g_pd3dDeviceContext->RSSetViewports(1, &vp);

	return true;
}

bool renderStateEdit(const D3D11_FILL_MODE& fm)
{
	if (rasterizer)
		rasterizer->Release();

	D3D11_RASTERIZER_DESC wfd;
	ZeroMemory(&wfd, sizeof wfd);
	wfd.FillMode = fm;
	wfd.CullMode = D3D11_CULL_BACK;
	wfd.DepthClipEnable = true;

	HRESULT hr = g_pd3dDevice->CreateRasterizerState(&wfd, &rasterizer);
	if (hr != S_OK)
		return false;

	return true;
}

int resizeWindow(SIZE p)
{
	if (!isRender)
		return 0;

	if (IsIconic(hWnd))
		return 0;

	video_settings.screen_width = p.cx;
	video_settings.screen_height = p.cy;

	if (!createTargetRender())
		return -1;

	//SendMessage(hWnd, WM_RENDER, NULL, NULL);
	return 1;
}

void clear()
{
	if (g_pSwapChain)
		g_pSwapChain->Release();

	if (g_pd3dDevice)
		g_pd3dDevice->Release();

	if (g_pd3dDeviceContext)
		g_pd3dDeviceContext->Release();

	if (backBuffer)
		backBuffer->Release();

	if (depthStancil)
		depthStancil->Release();

	if (depthStancilBuffer)
		depthStancilBuffer->Release();

	if (rasterizer)
		rasterizer->Release();

	DestroyWindow(hWnd);
	UnregisterClass(L"KOMOKSOFTWARE", hInstance);
}
#endif