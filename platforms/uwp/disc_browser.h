// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <SDL3/SDL.h>
#include <functional>
#include <string>
namespace Rml {class Context;}
namespace melee::uwp {
void choose_disc(Rml::Context*,std::function<void(std::string,std::string)>);
bool disc_browser_active();
bool disc_browser_event(const SDL_Event&);
void disc_browser_tick();
void close_disc_browser();
}
