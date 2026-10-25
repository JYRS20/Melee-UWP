// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdarg>
#include <cstdio>
#include <cstddef>
// GCC's Windows varargs use the same x64 ABI as the Store CRT. Keep these
// adapters outside the game sources and use the UCRT formatting routines.
extern "C" {
int __mingw_printf(const char* format,...) {
    va_list args;va_start(args,format);int result=std::vprintf(format,args);va_end(args);return result;
}
int __mingw_snprintf(char* output,size_t size,const char* format,...) {
    va_list args;va_start(args,format);int result=std::vsnprintf(output,size,format,args);va_end(args);return result;
}
int __mingw_vsnprintf(char* output,size_t size,const char* format,va_list args) {
    return std::vsnprintf(output,size,format,args);
}
int __mingw_sprintf(char* output,const char* format,...) {
    va_list args;va_start(args,format);int result=std::vsprintf(output,format,args);va_end(args);return result;
}
int __mingw_sscanf(const char* input,const char* format,...) {
    va_list args;va_start(args,format);int result=std::vsscanf(input,format,args);va_end(args);return result;
}
}

// GCC may combine a sin/cos pair and lower popcount to runtime helpers.
#include <cmath>
#include <cstdint>
#include <winsock2.h>
#include <windows.h>
extern "C" void sincos(double x,double* sine,double* cosine){*sine=std::sin(x);*cosine=std::cos(x);}
extern "C" int __popcountdi2(uint64_t value){int count=0;while(value){value&=value-1;++count;}return count;}
extern "C" int gettimeofday(timeval* output,void*){
 if(!output)return -1;
 FILETIME time;GetSystemTimeAsFileTime(&time);
 uint64_t ticks=(uint64_t(time.dwHighDateTime)<<32)|time.dwLowDateTime;
 ticks-=116444736000000000ULL;
 output->tv_sec=long(ticks/10000000);output->tv_usec=long((ticks%10000000)/10);return 0;
}
