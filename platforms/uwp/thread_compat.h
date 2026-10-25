// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/* Only the mutex/once subset used by the original audio and OS modules.
 * SDL mutexes are recursive, matching the game's interrupt/audio locks. */
#include <SDL3/SDL_mutex.h>
#include <errno.h>
#include <stdlib.h>
typedef struct { SDL_InitState once; SDL_Mutex* handle; } pthread_mutex_t;
typedef SDL_InitState pthread_once_t;
typedef int pthread_mutexattr_t;
#define PTHREAD_ONCE_INIT {0}
#define PTHREAD_MUTEX_INITIALIZER {0}
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP {0}
#define PTHREAD_MUTEX_RECURSIVE 1
static inline int pthread_mutexattr_init(pthread_mutexattr_t* a){*a=1;return 0;}
static inline int pthread_mutexattr_settype(pthread_mutexattr_t* a,int t){*a=t;return 0;}
static inline int pthread_mutexattr_destroy(pthread_mutexattr_t* a){(void)a;return 0;}
static inline int pthread_mutex_init(pthread_mutex_t* m,const pthread_mutexattr_t* a){
 (void)a;
 if(SDL_ShouldInit(&m->once)){m->handle=SDL_CreateMutex();SDL_SetInitialized(&m->once,m->handle!=NULL);}
 return m->handle?0:ENOMEM;
}
static inline int pthread_mutex_lock(pthread_mutex_t* m){if(pthread_mutex_init(m,NULL))abort();SDL_LockMutex(m->handle);return 0;}
static inline int pthread_mutex_unlock(pthread_mutex_t* m){SDL_UnlockMutex(m->handle);return 0;}
static inline int pthread_once(pthread_once_t* state,void (*fn)(void)){
 if(SDL_ShouldInit(state)){fn();SDL_SetInitialized(state,true);}return 0;
}
