#pragma once

const char* ScreenMode[] =
{
	"Fullscreen",
	"Windowed"
};

#ifdef DCS_OPENGL
const char* RenderMode[] =
{
	"OpenGL"
};
#else
const char* RenderMode[] =
{
	"DirectX 11"
};
#endif

const char* TextureQuality[] =
{
	"Low",
	"High",
};

const char* MenuLanguage[] =
{
	"English",
	"Russian",
};