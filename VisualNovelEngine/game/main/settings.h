#pragma once

struct VideoSettings_t
{
	bool vsync;
	//bool multithread;

	int screen_mode;

	int screen_width;
	int screen_height;

	int render_mode;
	int texture_quality;
};

struct AudioSettings_t
{
	int music_volume;
	int sound_volume;
};

struct GameSettings_t
{
	bool auto_save;
	bool show_fps_counter;

	bool animated_dialogue_text;
	int text_animation_speed;

	int menu_language;
};