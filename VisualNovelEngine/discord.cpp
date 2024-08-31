#include "discord.h"

void RPC_Initialize(const char* ApplicationId)
{
	HMODULE module = LoadLibraryW(L"rpc.dll");

	if (!module)
		return;

	FARPROC function = GetProcAddress(module, "?RPC_Initialize@@YAXPBD@Z");

	if (!function)
		return;

	typedef void (*RPC_Init)(const char*);

	((RPC_Init)function)(ApplicationId);
}

void RPC_UpdatePresence(DiscordRichPresence& discordPresence)
{
	HMODULE module = LoadLibraryW(L"rpc.dll");

	if (!module)
		return;

	FARPROC function = GetProcAddress(module, "?RPC_UpdatePresence@@YAXAAUDiscordRichPresence@@@Z");

	if (!function)
		return;

	typedef void (*RPC_UpdatePresence)(DiscordRichPresence&);
	((RPC_UpdatePresence)function)(discordPresence);
}