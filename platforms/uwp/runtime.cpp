// SPDX-License-Identifier: GPL-3.0-or-later
#include "runtime.h"
#include "storage.h"
#include <SDL3/SDL.h>
#include <windows.h>
#include <fileapifromapp.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.System.h>
#include <filesystem>
#include <thread>
#include <exception>
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#include <sqlite3.h>
extern "C" void pc_log_line(const char*,...);
namespace {
std::string preferences;
HANDLE startup_log=INVALID_HANDLE_VALUE;
std::string utf8(const std::filesystem::path& p){auto s=p.u8string();return {reinterpret_cast<const char*>(s.data()),s.size()};}
std::wstring native(const std::string& s){auto p=std::filesystem::u8path(s);p.make_preferred();return p.wstring();}
}
namespace melee::uwp {
static DWORD WINAPI attributes(LPCWSTR path){WIN32_FILE_ATTRIBUTE_DATA d{};return GetFileAttributesExFromAppW(path,GetFileExInfoStandard,&d)?d.dwFileAttributes:INVALID_FILE_ATTRIBUTES;}
static void initialize_sqlite(){
 auto vfs=sqlite3_vfs_find(nullptr);
 if(!vfs||vfs->iVersion<3||!vfs->xSetSystemCall)throw std::runtime_error("SQLite WinRT VFS is unavailable.");
 const struct {const char* name;sqlite3_syscall_ptr function;} calls[]{
  {"CreateFile2",reinterpret_cast<sqlite3_syscall_ptr>(CreateFile2FromAppW)},
  {"GetFileAttributesExW",reinterpret_cast<sqlite3_syscall_ptr>(GetFileAttributesExFromAppW)},
  {"GetFileAttributesW",reinterpret_cast<sqlite3_syscall_ptr>(attributes)},
  {"DeleteFileW",reinterpret_cast<sqlite3_syscall_ptr>(DeleteFileFromAppW)}
 };
 for(auto call:calls)if(vfs->xSetSystemCall(vfs,call.name,call.function)!=SQLITE_OK)throw std::runtime_error("Cannot configure SQLite USB access.");
}
void initialize_storage(){
 std::exception_ptr failure;
 std::thread worker([&]{
  winrt::init_apartment(winrt::apartment_type::multi_threaded);
  try{
   auto drives=usb_drives().get();if(drives.Size()==0)throw std::runtime_error("Connect a USB drive before starting Melee UWP. Game data is stored on USB.");
   auto selected=drives.GetAt(0);
   try{auto previous=winrt::Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList().GetFolderAsync(L"melee-usb-root").get();
    for(auto const& drive:drives)if(CompareStringOrdinal(drive.Path().c_str(),-1,previous.Path().c_str(),-1,TRUE)==CSTR_EQUAL){selected=drive;break;}
   }catch(winrt::hresult_error const&){}
   auto data=prepare_portable_root(selected).get();
   winrt::Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList().AddOrReplace(L"melee-usb-root",selected);
   auto root=std::filesystem::path(data.Path().c_str());
   preferences=utf8(root/L"user")+"/";
   startup_log=CreateFile2FromAppW((root/L"logs"/L"startup.log").c_str(),GENERIC_WRITE,FILE_SHARE_READ,CREATE_ALWAYS,nullptr);
   melee_uwp_note("Melee UWP GameTest 0.0.2.0; USB storage initialized");
   _putenv_s("MELEE_CACHE_DIR",utf8(root/L"cache").c_str());
   _putenv_s("MELEE_LOG_FILE",utf8(root/L"logs"/L"melee.log").c_str());
   _putenv_s("MELEE_BACKEND","d3d12");
   SDL_SetHint(SDL_HINT_JOYSTICK_WGI,"1");
   SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT,"0");
   // UWP cannot set its working directory to removable storage. All game-data paths are absolute.
  }catch(...){failure=std::current_exception();}
  winrt::uninit_apartment();
 });worker.join();if(failure)std::rethrow_exception(failure);initialize_sqlite();
 SDL_SetLogOutputFunction([](void*,int category,SDL_LogPriority,const char* message){pc_log_line("SDL[%d]: %s",category,message);},nullptr);
}
std::string read_text(const std::string& path){
 auto stream=SDL_IOFromFile(path.c_str(),"rb");if(!stream)return {};
 auto length=SDL_GetIOSize(stream);std::string text;
 if(length>=0&&length<=1024*1024){text.resize(size_t(length));if(SDL_ReadIO(stream,text.data(),text.size())!=text.size())text.clear();}
 SDL_CloseIO(stream);return text;
}
bool write_text_atomic(const std::string& path,const std::string& text){
 std::wstring temp;HANDLE file=INVALID_HANDLE_VALUE;
 auto target=native(path);
 for(int n=0;n<32&&file==INVALID_HANDLE_VALUE;++n){temp=target+L".tmp-"+std::to_wstring(SDL_GetTicksNS())+L"-"+std::to_wstring(n);file=CreateFile2FromAppW(temp.c_str(),GENERIC_WRITE,0,CREATE_NEW,nullptr);}
 if(file==INVALID_HANDLE_VALUE)return false;
 DWORD written=0;bool ok=text.size()<=MAXDWORD&&WriteFile(file,text.data(),DWORD(text.size()),&written,nullptr)&&written==text.size();
 if(ok)ok=FlushFileBuffers(file);CloseHandle(file);
 if(ok){
  WIN32_FILE_ATTRIBUTE_DATA existing{};
  if(GetFileAttributesExFromAppW(target.c_str(),GetFileExInfoStandard,&existing))ok=ReplaceFileFromAppW(target.c_str(),temp.c_str(),nullptr,0,nullptr,nullptr);
  else ok=MoveFileFromAppW(temp.c_str(),target.c_str());
 }
 if(!ok)DeleteFileFromAppW(temp.c_str());return ok;
}
bool regular_file(const std::string& path){
 WIN32_FILE_ATTRIBUTE_DATA data{};
 return GetFileAttributesExFromAppW(native(path).c_str(),GetFileExInfoStandard,&data)&&!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY);
}
std::string file_identity(const std::string& path){
 WIN32_FILE_ATTRIBUTE_DATA d{};
 if(!GetFileAttributesExFromAppW(native(path).c_str(),GetFileExInfoStandard,&d)||d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)return {};
 return path+":"+std::to_string((uint64_t(d.nFileSizeHigh)<<32)|d.nFileSizeLow)+":"+std::to_string((uint64_t(d.ftLastWriteTime.dwHighDateTime)<<32)|d.ftLastWriteTime.dwLowDateTime);
}
}
extern "C" const char* melee_uwp_pref_path(){return preferences.empty()?nullptr:preferences.c_str();}
extern "C" void* melee_uwp_open_lock(const char* path){
 CREATEFILE2_EXTENDED_PARAMETERS parameters{};parameters.dwSize=sizeof(parameters);parameters.dwFileAttributes=FILE_ATTRIBUTE_NORMAL;
 return CreateFile2FromAppW(native(path).c_str(),GENERIC_READ|GENERIC_WRITE,0,OPEN_ALWAYS,&parameters);
}

extern "C" uint64_t melee_uwp_memory_budget(){return winrt::Windows::System::MemoryManager::AppMemoryUsageLimit();}

extern "C" void melee_uwp_note(const char* message){
 OutputDebugStringA(message);
 if(startup_log!=INVALID_HANDLE_VALUE){DWORD written;WriteFile(startup_log,message,DWORD(strlen(message)),&written,nullptr);WriteFile(startup_log,"\r\n",2,&written,nullptr);FlushFileBuffers(startup_log);}
}

extern "C" FILE* melee_uwp_fopen(const char* path,const char* mode){
 if(!path||!mode){errno=EINVAL;return nullptr;}
 DWORD access=0,disposition=OPEN_EXISTING;int flags=0;
 bool plus=strchr(mode,'+')!=nullptr;
 if(mode[0]=='r'){access=GENERIC_READ;flags=plus?_O_RDWR:_O_RDONLY;}
 else if(mode[0]=='w'){access=GENERIC_WRITE;disposition=strchr(mode,'x')?CREATE_NEW:CREATE_ALWAYS;flags=plus?_O_RDWR:_O_WRONLY;}
 else if(mode[0]=='a'){access=GENERIC_WRITE;disposition=OPEN_ALWAYS;flags=(plus?_O_RDWR:_O_WRONLY)|_O_APPEND;}
 else{errno=EINVAL;return nullptr;}
 if(plus)access=GENERIC_READ|GENERIC_WRITE;
 flags|=strchr(mode,'b')?_O_BINARY:_O_TEXT;
 auto handle=CreateFile2FromAppW(native(path).c_str(),access,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,disposition,nullptr);
 if(handle==INVALID_HANDLE_VALUE){errno=GetLastError()==ERROR_FILE_NOT_FOUND?ENOENT:EACCES;return nullptr;}
 int fd=_open_osfhandle(intptr_t(handle),flags);
 if(fd<0){CloseHandle(handle);return nullptr;}
 FILE* file=_fdopen(fd,mode);if(!file)_close(fd);return file;
}
