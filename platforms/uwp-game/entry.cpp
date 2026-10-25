// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <windows.h>
#include "runtime.h"
#include <exception>
#include <csignal>
#include <cstdio>
#include <winrt/base.h>
extern "C" int aurora_main(int,char**);
extern "C" IMAGE_DOS_HEADER __ImageBase;
static void abort_trace(int){
 melee_uwp_note("SIGABRT observed in Melee UWP");
 void* frames[24];auto count=CaptureStackBackTrace(0,24,frames,nullptr);char line[160];
 for(USHORT n=0;n<count;++n){std::snprintf(line,sizeof(line),"stack[%u]=%p image_offset=0x%llx",unsigned(n),frames[n],static_cast<unsigned long long>(uintptr_t(frames[n])-uintptr_t(&__ImageBase)));melee_uwp_note(line);}
}
static void invalid_parameter(const wchar_t* expression,const wchar_t* function,const wchar_t*,unsigned,uintptr_t){
 char detail[1024];std::snprintf(detail,sizeof(detail),"UCRT invalid parameter: %ls in %ls",expression?expression:L"?",function?function:L"?");melee_uwp_note(detail);abort_trace(SIGABRT);std::abort();
}
static int run(int,char**){
 _set_invalid_parameter_handler(invalid_parameter);
 std::signal(SIGABRT,abort_trace);
 try{melee::uwp::initialize_storage();melee_uwp_note("Starting original Melee host and launcher");char app[]="melee-uwp";char* args[]={app,nullptr};int result=aurora_main(1,args);melee_uwp_note("Melee host returned normally");return result;}
 catch(winrt::hresult_error const& e){melee_uwp_note(winrt::to_string(e.message()).c_str());return 1;}
 catch(std::exception const& e){melee_uwp_note(e.what());SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Melee UWP",e.what(),nullptr);return 1;}
 catch(...){melee_uwp_note("Melee UWP startup failed");return 1;}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){return SDL_RunApp(0,nullptr,run,nullptr);}
