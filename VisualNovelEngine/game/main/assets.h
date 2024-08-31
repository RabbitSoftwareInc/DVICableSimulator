#pragma once
#include <string>
#include <Windows.h>
#include <D3DX11.h>
#include <vector>

#include <SFML/Graphics.hpp>

#include "../config.h"

class ImFont;

struct Image_t
{
#ifndef DCS_OPENGL
	ID3D11ShaderResourceView* image_data;
#else
	sf::Texture image_data;
#endif

	std::wstring image_name;
};

struct Font_t
{
	Font_t()
	{
	}

	Font_t(ImFont* f, int s, std::wstring n)
	{
		font_data = f;
		font_size = s;

		font_name = n;
	}

	ImFont* font_data;
	int font_size;

	std::wstring font_name;
};

struct MusicData_t
{
	MusicData_t()
	{
	}

	std::wstring music_path;
	std::wstring music_name;

	bool is_valid_music()
	{
		if (music_name != L"NONE" && music_path != L"")
			return true;

		return false;
	}
};

struct GameMenuData_t
{
	Image_t game_logo;
	Image_t intro_logo;

	Image_t main_menu_background;
	//MusicData_t main_menu_background_music;
};

struct GameFonts_t
{
	Font_t intro_font;
	Font_t main_menu_font;

	Font_t dialogue_name_font;
	Font_t dialogue_text_font;
};

struct GameInfo_t
{
	std::wstring game_name;
	std::wstring game_developer;

	std::wstring game_rpc_app_id;
	std::wstring game_rpc_app_logo;
};