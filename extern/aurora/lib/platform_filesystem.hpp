// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#ifndef AURORA_WINDOWS_STORE
namespace aurora { namespace platformfs=std::filesystem; }
#else
#include <windows.h>
#include <fileapifromapp.h>
#include <memory>
#include <vector>
namespace aurora::platformfs {
using namespace std::filesystem;
inline file_status symlink_status(const path& p,std::error_code& ec){
 WIN32_FILE_ATTRIBUTE_DATA d{};
 if(!GetFileAttributesExFromAppW(p.c_str(),GetFileExInfoStandard,&d)){
  auto e=GetLastError();if(e==ERROR_FILE_NOT_FOUND||e==ERROR_PATH_NOT_FOUND){ec.clear();return file_status(file_type::not_found);}
  ec={int(e),std::system_category()};return file_status(file_type::none);
 }
 ec.clear();return file_status((d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?file_type::directory:file_type::regular);
}
inline bool exists(file_status status){return status.type()!=file_type::not_found&&status.type()!=file_type::none;}
inline bool exists(const path& p,std::error_code& ec){return aurora::platformfs::exists(aurora::platformfs::symlink_status(p,ec));}
inline bool exists(const path& p){std::error_code ec;return aurora::platformfs::exists(p,ec);}
inline bool is_directory(const path& p,std::error_code& ec){return aurora::platformfs::symlink_status(p,ec).type()==file_type::directory;}
inline bool is_directory(const path& p){std::error_code ec;return aurora::platformfs::is_directory(p,ec);}
inline bool is_regular_file(const path& p,std::error_code& ec){return aurora::platformfs::symlink_status(p,ec).type()==file_type::regular;}
inline uintmax_t file_size(const path& p){WIN32_FILE_ATTRIBUTE_DATA d{};if(!GetFileAttributesExFromAppW(p.c_str(),GetFileExInfoStandard,&d))throw filesystem_error("Cannot read USB file size",p,std::error_code(int(GetLastError()),std::system_category()));return (uint64_t(d.nFileSizeHigh)<<32)|d.nFileSizeLow;}
class directory_entry {
 std::filesystem::path value;file_status state;
public:
 directory_entry(std::filesystem::path p,DWORD attributes):value(std::move(p)),state((attributes&FILE_ATTRIBUTE_DIRECTORY)?file_type::directory:file_type::regular){}
 const std::filesystem::path& path()const{return value;}
 file_status status(std::error_code& ec)const{ec.clear();return state;}
 bool is_regular_file()const{return state.type()==file_type::regular;}
};
inline void collect(const path& root,std::vector<directory_entry>& out,bool recursive,std::error_code& ec,unsigned depth=0){
 WIN32_FIND_DATAW data{};auto handle=FindFirstFileExFromAppW((root/L"*").c_str(),FindExInfoStandard,&data,FindExSearchNameMatch,nullptr,0);
 if(handle==INVALID_HANDLE_VALUE){auto e=GetLastError();if(e!=ERROR_FILE_NOT_FOUND)ec={int(e),std::system_category()};return;}
 do{
  if(wcscmp(data.cFileName,L".")==0||wcscmp(data.cFileName,L"..")==0)continue;
  // Never follow reparse points out of the chosen removable-storage tree.
  if(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)continue;
  auto p=root/data.cFileName;out.emplace_back(p,data.dwFileAttributes);
  if(recursive&&(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&depth<128){collect(p,out,true,ec,depth+1);if(ec)break;}
 }while(FindNextFileW(handle,&data));
 auto last=GetLastError();FindClose(handle);if(!ec&&last!=ERROR_NO_MORE_FILES)ec={int(last),std::system_category()};
}
template<bool Recursive>class iterator {
 std::shared_ptr<std::vector<directory_entry>> values;size_t index=0;
 bool ended()const{return !values||index>=values->size();}
public:
 iterator()=default;
 iterator(const path& root,std::error_code& ec):values(std::make_shared<std::vector<directory_entry>>()){ec.clear();collect(root,*values,Recursive,ec);}
 iterator(const path& root,directory_options,std::error_code& ec):iterator(root,ec){}
 const directory_entry& operator*()const{return (*values)[index];}
 const directory_entry* operator->()const{return &(*values)[index];}
 iterator& increment(std::error_code& ec){ec.clear();++index;return *this;}
 bool operator!=(const iterator& other)const{return !(ended()&&other.ended())&&(values!=other.values||index!=other.index);}
};
using directory_iterator=iterator<false>;
using recursive_directory_iterator=iterator<true>;
}
#endif
