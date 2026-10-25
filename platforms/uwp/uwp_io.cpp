// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MELEE_UWP_IO_IMPLEMENTATION
#define MELEE_UWP_IO_IMPLEMENTATION
#endif
#include "uwp_io.h"
#include <windows.h>
#include <fileapifromapp.h>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <io.h>
extern "C" FILE* melee_uwp_fopen(const char*,const char*);
namespace {
std::wstring native(const char* s){auto p=std::filesystem::u8path(s);p.make_preferred();return p.wstring();}
bool error(){return SDL_SetError("USB filesystem error %lu",GetLastError());}
Sint64 SDLCALL size(void* f){return _filelengthi64(_fileno(static_cast<FILE*>(f)));}
Sint64 SDLCALL seek(void* f,Sint64 offset,SDL_IOWhence whence){auto file=static_cast<FILE*>(f);if(_fseeki64(file,offset,int(whence)))return -1;return _ftelli64(file);}
size_t SDLCALL read(void* f,void* out,size_t length,SDL_IOStatus* status){auto file=static_cast<FILE*>(f);auto count=fread(out,1,length,file);if(count<length)*status=ferror(file)?SDL_IO_STATUS_ERROR:SDL_IO_STATUS_EOF;return count;}
size_t SDLCALL write(void* f,const void* data,size_t length,SDL_IOStatus* status){auto count=fwrite(data,1,length,static_cast<FILE*>(f));if(count<length)*status=SDL_IO_STATUS_ERROR;return count;}
bool SDLCALL flush(void* f,SDL_IOStatus* status){auto file=static_cast<FILE*>(f);bool ok=fflush(file)==0&&FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file))));if(!ok)*status=SDL_IO_STATUS_ERROR;return ok;}
bool SDLCALL close(void* f){return fclose(static_cast<FILE*>(f))==0;}
bool mkdir(const std::filesystem::path& path,unsigned depth=0){
 WIN32_FILE_ATTRIBUTE_DATA data{};
 if(GetFileAttributesExFromAppW(path.c_str(),GetFileExInfoStandard,&data))return (data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0;
 if(CreateDirectoryFromAppW(path.c_str(),nullptr))return true;
 auto failure=GetLastError();auto parent=path.parent_path();
 if(failure==ERROR_PATH_NOT_FOUND&&depth<256&&!parent.empty()&&parent!=path&&mkdir(parent,depth+1))return CreateDirectoryFromAppW(path.c_str(),nullptr)!=FALSE;
 SetLastError(failure);return false;
}
}
extern "C" SDL_IOStream* melee_uwp_io_from_file(const char* path,const char* mode){
 FILE* file=melee_uwp_fopen(path,mode);if(!file){SDL_SetError("Cannot open %s: %s",path,strerror(errno));return nullptr;}
 SDL_IOStreamInterface callbacks;SDL_INIT_INTERFACE(&callbacks);
 callbacks.size=size;callbacks.seek=seek;callbacks.read=read;callbacks.write=write;callbacks.flush=flush;callbacks.close=close;
 auto stream=SDL_OpenIO(&callbacks,file);if(!stream)fclose(file);return stream;
}
extern "C" bool melee_uwp_create_directory(const char* path){if(mkdir(std::filesystem::path(native(path))))return true;return error();}
extern "C" bool melee_uwp_get_path_info(const char* path,SDL_PathInfo* info){
 WIN32_FILE_ATTRIBUTE_DATA d{};if(!GetFileAttributesExFromAppW(native(path).c_str(),GetFileExInfoStandard,&d))return error();
 if(info){*info={};info->type=(d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?SDL_PATHTYPE_DIRECTORY:SDL_PATHTYPE_FILE;info->size=(uint64_t(d.nFileSizeHigh)<<32)|d.nFileSizeLow;}
 return true;
}
extern "C" bool melee_uwp_remove_path(const char* path){
 auto p=native(path);WIN32_FILE_ATTRIBUTE_DATA d{};
 if(!GetFileAttributesExFromAppW(p.c_str(),GetFileExInfoStandard,&d))return error();
 if((d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?RemoveDirectoryFromAppW(p.c_str()):DeleteFileFromAppW(p.c_str()))return true;return error();
}
extern "C" bool melee_uwp_rename_path(const char* from,const char* to){
 auto a=native(from),b=native(to);WIN32_FILE_ATTRIBUTE_DATA d{};
 bool exists=GetFileAttributesExFromAppW(b.c_str(),GetFileExInfoStandard,&d)!=FALSE;
 bool ok=exists?ReplaceFileFromAppW(b.c_str(),a.c_str(),nullptr,0,nullptr,nullptr):MoveFileFromAppW(a.c_str(),b.c_str());return ok?true:error();
}
