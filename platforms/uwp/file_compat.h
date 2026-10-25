// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
FILE* melee_uwp_fopen(const char* path,const char* mode);
#ifdef __cplusplus
}
#endif
#define fopen melee_uwp_fopen

#include "uwp_io.h"
