#pragma once
#include <Windows.h>
#include <cstdint>

struct DiscordRichPresence
{
	const char* state;   /* max 128 bytes */
	const char* details; /* max 128 bytes */
	uint64_t startTimestamp;
	uint64_t endTimestamp;
	const char* largeImageKey;  /* max 32 bytes */
	const char* largeImageText; /* max 128 bytes */
	const char* smallImageKey;  /* max 32 bytes */
	const char* smallImageText; /* max 128 bytes */
	const char* partyId;        /* max 128 bytes */
	const char* button1_label, * button1_url; /* max 128 bytes */
	const char* button2_label, * button2_url; /* max 128 bytes */
	int partySize;
	int partyMax;
	const char* matchSecret;    /* max 128 bytes */
	const char* joinSecret;     /* max 128 bytes */
	const char* spectateSecret; /* max 128 bytes */
	char gf[1];
};

void RPC_Initialize(const char* ApplicationId);
void RPC_UpdatePresence(DiscordRichPresence& discordPresence);