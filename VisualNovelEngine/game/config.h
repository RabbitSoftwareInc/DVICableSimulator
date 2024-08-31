#pragma once

// defines
// --------------------------------------------------------------------- //
// #define DCS_CONFIG // DVI Cable Simulator configuration
// #define DDLC_CPP_CONFIG // Doki Doki Literature Club C++ configuration
// --------------------------------------------------------------------- //
// #define DCS_STORY_GAME // Story game mode. Removes scenario selector and automatically loads "main.sc" scenario. Enables advanced scenes by default. Adds story game specific features. Disables some discord rpc features.
// #define DCS_DEBUG // Debug mode. Allocates console on start.
//
// #define DISCORD_RPC // Enables discord rpc.
// --------------------------------------------------------------------- //
// #define DCS_OPENGL // Game uses OpenGL rendering.
// --------------------------------------------------------------------- //

#define DCS_CONFIG
#define DCS_OPENGL

#if defined(DCS_CONFIG)
#define DISCORD_RPC
#elif defined(DDLC_CPP_CONFIG)
#define DCS_STORY_GAME
#endif
