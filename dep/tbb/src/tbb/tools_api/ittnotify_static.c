/*
    Copyright 2005-2009 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#include "_config.h"

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#include <windows.h>
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#include <pthread.h>
#include <dlfcn.h>
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define __ITT_INTERNAL_INCLUDE

#define _ITTNOTIFY_H_MACRO_BODY_

#include "_disable_warnings.h"

#include "ittnotify.h"

#ifdef __cplusplus
#  define ITT_EXTERN_C extern "C"
#else
#  define ITT_EXTERN_C /* nothing */
#endif /* __cplusplus */

#ifndef __itt_init_lib_name
#  define __itt_init_lib_name __itt_init_lib
#endif /* __itt_init_lib_name */

static int __itt_init_lib(void);

#ifndef INTEL_ITTNOTIFY_PREFIX
#define INTEL_ITTNOTIFY_PREFIX __itt_
#endif /* INTEL_ITTNOTIFY_PREFIX */
#ifndef INTEL_ITTNOTIFY_POSTFIX
#  define INTEL_ITTNOTIFY_POSTFIX _ptr_
#endif /* INTEL_ITTNOTIFY_POSTFIX */

#define ___N_(p,n) p##n
#define __N_(p,n) ___N_(p,n)
#define _N_(n) __N_(INTEL_ITTNOTIFY_PREFIX,n)

/* building pointers to imported funcs */
#undef ITT_STUBV
#undef ITT_STUB
#define ITT_STUB(type,name,args,params,ptr,group)                       \
    static type ITTAPI_CALL ITT_JOIN(_N_(name),_init) args;             \
    typedef type ITTAPI_CALL name##_t args;                             \
    ITT_EXTERN_C name##_t* ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX) = ITT_JOIN(_N_(name),_init); \
    static type ITTAPI_CALL ITT_JOIN(_N_(name),_init) args              \
    {                                                                   \
        __itt_init_lib_name();                                          \
        if(ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX))                                             \
            return ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX) params;                              \
        else                                                            \
            return (type)0;                                             \
    }

#define ITT_STUBV(type,name,args,params,ptr,group)                      \
    static type ITTAPI_CALL ITT_JOIN(_N_(name),_init) args;             \
    typedef type ITTAPI_CALL name##_t args;                             \
    ITT_EXTERN_C name##_t* ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX) = ITT_JOIN(_N_(name),_init); \
    static type ITTAPI_CALL ITT_JOIN(_N_(name),_init) args              \
    {                                                                   \
        __itt_init_lib_name();                                          \
        if(ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX))                                             \
            ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX) params;                                     \
        else                                                            \
            return;                                                     \
    }

const __itt_state_t _N_(state_err) = 0;
const __itt_event _N_(event_err) = 0;
const int _N_(err) = 0;

#include "_ittnotify_static.h"

typedef enum ___itt_group_id
{
    __itt_none_group    = 0,
    __itt_control_group = 1,
    __itt_thread_group  = 2,
    __itt_mark_group    = 4,
    __itt_sync_group    = 8,
    __itt_fsync_group   = 16,
    __itt_jit_group     = 32,
    __itt_all_group     = -1
} __itt_group_id;


#ifndef CDECL
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#    define CDECL __cdecl
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#    define CDECL
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* CDECL */

#ifndef STDCALL
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#    define STDCALL __stdcall
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#    define STDCALL
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* STDCALL */

#if ITT_PLATFORM==ITT_PLATFORM_WIN
    typedef FARPROC FPTR;
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
    typedef void* FPTR;
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */


/* OS communication functions */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
typedef HMODULE lib_t;
typedef CRITICAL_SECTION mutex_t;
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
typedef void* lib_t;
typedef pthread_mutex_t mutex_t;
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

static lib_t ittnotify_lib;

static __itt_error_notification_t* error_handler = 0;

#if ITT_OS==ITT_OS_WIN
static const char* ittnotify_lib_name = "libittnotify.dll";
#elif ITT_OS==ITT_OS_LINUX
static const char* ittnotify_lib_name = "libittnotify.so";
#elif ITT_OS==ITT_OS_MAC
static const char* ittnotify_lib_name = "libittnotify.dylib";
#else
#error Unsupported or unknown OS.
#endif

#ifndef LIB_VAR_NAME
#if ITT_ARCH==ITT_ARCH_IA32
#define LIB_VAR_NAME INTEL_LIBITTNOTIFY32
#else
#define LIB_VAR_NAME INTEL_LIBITTNOTIFY64
#endif
#endif /* LIB_VAR_NAME */

#define __TO_STR(x) #x
#define _TO_STR(x) __TO_STR(x)

static int __itt_fstrcmp(const char* s1, const char* s2)
{
    int i;

    if(!s1 && !s2)
        return 0;
    else if(!s1 && s2)
        return -1;
    else if(s1 && !s2)
        return 1;

    for(i = 0; s1[i] || s2[i]; i++)
        if(s1[i] > s2[i])
            return 1;
        else if(s1[i] < s2[i])
            return -1;
    return 0;
}

static const char* __itt_fsplit(const char* s, const char* sep, const char** out, int* len)
{
    int i;
    int j;

    if(!s || !sep || !out || !len)
        return 0;

    for(i = 0; s[i]; i++)
    {
        int b = 0;
        for(j = 0; sep[j]; j++)
            if(s[i] == sep[j])
            {
                b = 1;
                break;
            }
        if(!b)
            break;
    }

    if(!s[i])
        return 0;

    *len = 0;
    *out = s + i;

    for(; s[i]; i++, (*len)++)
    {
        int b = 0;
        for(j = 0; sep[j]; j++)
            if(s[i] == sep[j])
            {
                b = 1;
                break;
            }
        if(b)
            break;
    }

    for(; s[i]; i++)
    {
        int b = 0;
        for(j = 0; sep[j]; j++)
            if(s[i] == sep[j])
            {
                b = 1;
                break;
            }
        if(!b)
            break;
    }

    return s + i;
}

static char* __itt_fstrcpyn(char* dst, const char* src, int len)
{
    int i;

    if(!src || !dst)
        return 0;

    for(i = 0; i < len; i++)
        dst[i] = src[i];
    dst[len] = 0;
    return dst;
}

#ifdef ITT_NOTIFY_EXT_REPORT
#  define ERROR_HANDLER ITT_JOIN(INTEL_ITTNOTIFY_PREFIX, error_handler)
ITT_EXTERN_C void ERROR_HANDLER(__itt_error_code, const char* msg);
#endif /* ITT_NOTIFY_EXT_REPORT */

static void __itt_report_error(__itt_error_code code, const char* msg)
{
    if(error_handler)
        error_handler(code, msg);
#ifdef ITT_NOTIFY_EXT_REPORT
    ERROR_HANDLER(code, msg);
#endif /* ITT_NOTIFY_EXT_REPORT */
}

static const char* __itt_get_env_var(const char* name)
{
    static char env_value[4096];
#if ITT_PLATFORM==ITT_PLATFORM_WIN
    int i;
    DWORD rc;
    for(i = 0; i < sizeof(env_value); i++)
        env_value[i] = 0;
    rc = GetEnvironmentVariableA(name, env_value, sizeof(env_value) - 1);
    if(rc >= sizeof(env_value))
        __itt_report_error(__itt_error_cant_read_env, name);
    else if(!rc)
        return 0;
    else
        return env_value;
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
    char* env = getenv(name);
    int i;
    for(i = 0; i < sizeof(env_value); i++)
        env_value[i] = 0;
    if(env)
    {
        if(strlen(env) >= sizeof(env_value))
        {
            __itt_report_error(__itt_error_cant_read_env, name);
            return 0;
        }
        strncpy(env_value, env, sizeof(env_value) - 1);
        return env_value;
    }
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
    return 0;
}

static const char* __itt_get_lib_name()
{
    const char* lib_name = __itt_get_env_var(_TO_STR(LIB_VAR_NAME));
    if(!lib_name)
        lib_name = ittnotify_lib_name;

    return lib_name;
}

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#  define __itt_get_proc(lib, name) GetProcAddress(lib, name)
#  define __itt_init_mutex(mutex)   InitializeCriticalSection(mutex)
#  define __itt_mutex_lock(mutex)   EnterCriticalSection(mutex)
#  define __itt_mutex_unlock(mutex) LeaveCriticalSection(mutex)
#  define __itt_load_lib(name)      LoadLibraryA(name)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#  define __itt_get_proc(lib, name) dlsym(lib, name)
#  define __itt_init_mutex(mutex)   pthread_mutex_init(mutex, 0)
#  define __itt_mutex_lock(mutex)   pthread_mutex_lock(mutex)
#  define __itt_mutex_unlock(mutex) pthread_mutex_unlock(mutex)
#  define __itt_load_lib(name)      dlopen(name, RTLD_LAZY)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#ifndef ITT_SIMPLE_INIT
/* function stubs */

#undef ITT_STUBV
#undef ITT_STUB

#define ITT_STUBV(type,name,args,params,ptr,group) \
ITT_EXTERN_C type ITTAPI_CALL _N_(name) args                \
{                                                  \
    __itt_init_lib_name();                         \
    if(ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX))         \
        ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX) params; \
    else                                           \
        return;                                    \
}

#define ITT_STUB(type,name,args,params,ptr,group) \
ITT_EXTERN_C type ITTAPI_CALL _N_(name) args                        \
{                                                 \
    __itt_init_lib_name();                        \
    if(ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX))                 \
        return ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX) params;  \
    else                                          \
        return (type)0;                           \
}

#include "_ittnotify_static.h"

#endif /* ITT_SIMPLE_INIT */

typedef struct ___itt_group_list
{
    __itt_group_id id;
    const char*    name;
} __itt_group_list;

static __itt_group_list group_list[] = {
    {__itt_control_group, "control"},
    {__itt_thread_group,  "thread"},
    {__itt_mark_group,    "mark"},
    {__itt_sync_group,    "sync"},
    {__itt_fsync_group,   "fsync"},
    {__itt_jit_group,     "jit"},
    {__itt_all_group,     "all"},
    {__itt_none_group,    0}
};

typedef struct ___itt_group_alias
{
    const char*    env_var;
    __itt_group_id groups;
} __itt_group_alias;

static __itt_group_alias group_alias[] = {
    {"KMP_FOR_TPROFILE", (__itt_group_id)(__itt_control_group | __itt_thread_group | __itt_sync_group | __itt_mark_group)},
    {"KMP_FOR_TCHECK", (__itt_group_id)(__itt_control_group | __itt_thread_group | __itt_fsync_group | __itt_mark_group)},
    {0, __itt_none_group}
};

typedef struct ___itt_func_map
{
    const char*    name;
    void**         func_ptr;
    __itt_group_id group;
} __itt_func_map;


#define _P_(name) ITT_JOIN(_N_(name),INTEL_ITTNOTIFY_POSTFIX)

#define ITT_STRINGIZE_AUX(p) #p
#define ITT_STRINGIZE(p) ITT_STRINGIZE_AUX(p)

#define __ptr_(pname,name,group) {ITT_STRINGIZE(ITT_JOIN(__itt_,pname)), (void**)(void*)&_P_(name), (__itt_group_id)(group)},

#undef ITT_STUB
#undef ITT_STUBV

#define ITT_STUB(type,name,args,params,ptr,group) __ptr_(ptr,name,group)
#define ITT_STUBV ITT_STUB

static __itt_func_map func_map[] = {
#include "_ittnotify_static.h"
    {0, 0, __itt_none_group}
};

static __itt_group_id __itt_get_groups()
{
    __itt_group_id res = __itt_none_group;

    const char* group_str = __itt_get_env_var("INTEL_ITTNOTIFY_GROUPS");
    if(group_str)
    {
        char gr[255];
        const char* chunk;
        int len;
        while((group_str = __itt_fsplit(group_str, ",; ", &chunk, &len)) != 0)
        {
            int j;
            int group_detected = 0;
            __itt_fstrcpyn(gr, chunk, len);
            for(j = 0; group_list[j].name; j++)
            {
                if(!__itt_fstrcmp(gr, group_list[j].name))
                {
                    res = (__itt_group_id)(res | group_list[j].id);
                    group_detected = 1;
                    break;
                }
            }

            if(!group_detected)
                __itt_report_error(__itt_error_unknown_group, gr);
        }
        return res;
    }
    else
    {
        int i;
        for(i = 0; group_alias[i].env_var; i++)
            if(__itt_get_env_var(group_alias[i].env_var))
                return group_alias[i].groups;
    }

    return res;
}

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#pragma warning(push)
#pragma warning(disable: 4054)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

static int __itt_init_lib()
{
    static volatile int init = 0;
    static int result = 0;

#ifndef ITT_SIMPLE_INIT

#if ITT_PLATFORM==ITT_PLATFORM_POSIX
    static mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#else
    static volatile int mutex_initialized = 0;
    static mutex_t mutex;
    static LONG inter_counter = 0;
#endif

    if(!init)
    {
#if ITT_PLATFORM==ITT_PLATFORM_WIN
        if(!mutex_initialized)
        {
            if(InterlockedIncrement(&inter_counter) == 1)
            {
                __itt_init_mutex(&mutex);
                mutex_initialized = 1;
            }
            else
                while(!mutex_initialized)
                    SwitchToThread();
        }
#endif

        __itt_mutex_lock(&mutex);
#endif /* ITT_SIMPLE_INIT */
        if(!init)
        {
            int i;

            __itt_group_id groups = __itt_get_groups();

            for(i = 0; func_map[i].name; i++)
                *func_map[i].func_ptr = 0;

            if(groups != __itt_none_group)
            {
#ifdef ITT_COMPLETE_GROUP
                __itt_group_id zero_group = __itt_none_group;
#endif /* ITT_COMPLETE_GROUP */

                ittnotify_lib = __itt_load_lib(__itt_get_lib_name());
                if(ittnotify_lib)
                {
                    for(i = 0; func_map[i].name; i++)
                    {
                        if(func_map[i].name && func_map[i].func_ptr && (func_map[i].group & groups))
                        {
                            *func_map[i].func_ptr = (void*)__itt_get_proc(ittnotify_lib, func_map[i].name);
                            if(!(*func_map[i].func_ptr) && func_map[i].name)
                            {
                                __itt_report_error(__itt_error_no_symbol, func_map[i].name);
#ifdef ITT_COMPLETE_GROUP
                                zero_group = (__itt_group_id)(zero_group | func_map[i].group);
#endif /* ITT_COMPLETE_GROUP */
                            }
                            else
                                result = 1;
                        }
                    }
                }
                else
                {
                    __itt_report_error(__itt_error_no_module, __itt_get_lib_name());
                }

#ifdef ITT_COMPLETE_GROUP
                for(i = 0; func_map[i].name; i++)
                    if(func_map[i].group & zero_group)
                        *func_map[i].func_ptr = 0;

                result = 0;

                for(i = 0; func_map[i].name; i++) /* evaluating if any function ptr is non empty */
                    if(*func_map[i].func_ptr)
                    {
                        result = 1;
                        break;
                    }
#endif /* ITT_COMPLETE_GROUP */
            }

            init = 1; /* first checking of 'init' flag happened out of mutex, that is why setting flag to 1 */
                      /* must be after call table is filled (to avoid condition races) */
        }
#ifndef ITT_SIMPLE_INIT
        __itt_mutex_unlock(&mutex);
    }
#endif /* ITT_SIMPLE_INIT */
    return result;
}

#define SET_ERROR_HANDLER ITT_JOIN(INTEL_ITTNOTIFY_PREFIX, set_error_handler)

ITT_EXTERN_C __itt_error_notification_t* SET_ERROR_HANDLER(__itt_error_notification_t* handler)
{
    __itt_error_notification_t* prev = error_handler;
    error_handler = handler;
    return prev;
}

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#pragma warning(pop)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
