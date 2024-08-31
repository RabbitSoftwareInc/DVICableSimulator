#pragma once
#include <string>
#include <Windows.h>
#include <D3DX11.h>
#include <vector>

#include <SFML/Graphics.hpp>

#include "../config.h"

struct ScenarioTexture_t
{
#ifndef DCS_OPENGL
	ID3D11ShaderResourceView* texture_data;
#else
	sf::Texture texture_data;
#endif

	std::wstring texture_name;
};

struct ScenarioDialogueScenePersonData_t
{
	std::wstring person_name;
	std::wstring person_texture;

	std::wstring person_texture_left;
	std::wstring person_texture_right;
	std::wstring person_texture_head;

	bool talking;
	std::wstring talking_text;

	bool is_valid_name()
	{
		if (person_name != L"NONE")
			return true;

		return false;
	}

	bool is_valid_texture()
	{
		if (person_texture != L"NONE")
			return true;

		return false;
	}

	bool is_valid_texture_left()
	{
		if (person_texture_left != L"NONE")
			return true;

		return false;
	}

	bool is_valid_texture_right()
	{
		if (person_texture_right != L"NONE")
			return true;

		return false;
	}

	bool is_valid_texture_head()
	{
		if (person_texture_head != L"NONE")
			return true;

		return false;
	}
};

struct ScenarioDialogueSceneScriptAction_t
{

};

struct ScenarioDialogueSceneButton_t
{
	std::wstring button_name;
	std::wstring button_scenario_to_load;

	ScenarioDialogueSceneScriptAction_t button_script_callback;

	bool is_present()
	{
		if (button_name != L"NONE")
			return true;

		return false;
	}

	bool has_scenario_action()
	{
		if (button_scenario_to_load != L"NONE")
			return true;

		return false;
	}
};

struct ScenarioDialogueScene_t
{
	ScenarioDialogueScene_t()
	{
	};

	std::wstring background_texture;
	std::wstring background_overlay_texture;

	std::wstring background_music;

	std::wstring additional_scene_sound;

	ScenarioDialogueScenePersonData_t person1;
	ScenarioDialogueScenePersonData_t person2;
	ScenarioDialogueScenePersonData_t person3;
	ScenarioDialogueScenePersonData_t person4;

	ScenarioDialogueScenePersonData_t main_character;

	ScenarioDialogueSceneButton_t button1;
	ScenarioDialogueSceneButton_t button2;
	ScenarioDialogueSceneButton_t button3;
	ScenarioDialogueSceneButton_t button4;

	std::wstring overlay_texture;
};

struct Scenario_t
{
	std::wstring music_dir;
	std::wstring textures_dir;

	std::wstring file_path;
	std::wstring file_name;

	std::vector<ScenarioTexture_t> textures;
	std::vector<ScenarioDialogueScene_t> scenes;

	bool loaded;
};

struct GameSave_t
{
	std::wstring scenario_name;
	std::wstring player_name;

	int scenario_scene;
};