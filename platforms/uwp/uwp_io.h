// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_filesystem.h>
#ifdef __cplusplus
extern "C" {
#endif
SDL_IOStream* melee_uwp_io_from_file(const char*,const char*);
bool melee_uwp_create_directory(const char*);
bool melee_uwp_get_path_info(const char*,SDL_PathInfo*);
bool melee_uwp_rename_path(const char*,const char*);
bool melee_uwp_remove_path(const char*);
#ifdef __cplusplus
}
#endif
#ifndef MELEE_UWP_IO_IMPLEMENTATION
#define SDL_IOFromFile melee_uwp_io_from_file
#define SDL_CreateDirectory melee_uwp_create_directory
#define SDL_GetPathInfo melee_uwp_get_path_info
#define SDL_RenamePath melee_uwp_rename_path
#define SDL_RemovePath melee_uwp_remove_path
#endif
