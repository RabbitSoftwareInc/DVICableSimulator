#pragma once
#include <string>
#include <Windows.h>
#include <D3DX11.h>
#include <vector>

struct TranslatedText_t
{
	std::wstring original_text;
	std::wstring translation_text;
};

struct GameLanguage_t
{
	std::wstring name;
	std::wstring codepage;

	std::vector<TranslatedText_t> strings;
};