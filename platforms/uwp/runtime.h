// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#ifdef __cplusplus
#include <string>
namespace melee::uwp {
void initialize_storage();
std::string file_identity(const std::string&);
bool regular_file(const std::string&);
std::string read_text(const std::string&);
bool write_text_atomic(const std::string&,const std::string&);
}
extern "C" {
#endif
const char* melee_uwp_pref_path(void);
void melee_uwp_note(const char* message);
uint64_t melee_uwp_memory_budget(void);
#ifdef __cplusplus
}
#endif
