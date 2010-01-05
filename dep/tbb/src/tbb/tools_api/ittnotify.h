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

/** @mainpage
 * Ability to control the collection during runtime. User API can be inserted into the user application.
 * Commands include:
	- Pause/resume analysis
	- Stop analysis and application, view results
	- Cancel analysis and application without generating results
	- Mark current time in results
 * The User API provides ability to control the collection, set marks at the execution of specific user code and
 * specify custom synchronization primitives implemented without standard system APIs. 
 * 
 * Use case: User inserts API calls to the desired places in her code. The code is then compiled and
 * linked with static part of User API library. User can recompile the code with specific macro defined 
 * to enable API calls.  If this macro is not defined there is no run-time overhead and no need to  link 
 * with static part of User API library. During  runtime the static library loads and initializes the dynamic part.
 * In case of instrumentation-based collection, only a stub library is loaded; otherwise a proxy library is loaded,
 * which calls the collector.
 * 
 * User API set is native (C/C++) only (no MRTE support). As amitigation can use JNI or C/C++ function 
 * call from managed code where needed. If the collector causes significant overhead or data storage, then 
 * pausing analysis should reduce the overhead to minimal levels.
*/
/** @example example.cpp
 * @brief The following example program shows the usage of User API
 */

#ifndef _ITTNOTIFY_H_
#define _ITTNOTIFY_H_
/** @file ittnotify.h
 *  @brief Header file which contains declaration of user API functions and types
 */

/** @cond exclude_from_documentation */
#ifndef ITT_OS_WIN
#  define ITT_OS_WIN   1
#endif /* ITT_OS_WIN */

#ifndef ITT_OS_LINUX
#  define ITT_OS_LINUX 2
#endif /* ITT_OS_LINUX */

#ifndef ITT_OS_MAC
#  define ITT_OS_MAC   3
#endif /* ITT_OS_MAC */

#ifndef ITT_OS
#  if defined WIN32 || defined _WIN32
#    define ITT_OS ITT_OS_WIN
#  elif defined( __APPLE__ ) && defined( __MACH__ )
#    define ITT_OS ITT_OS_MAC
#  else
#    define ITT_OS ITT_OS_LINUX
#  endif
#endif /* ITT_OS */

#ifndef ITT_PLATFORM_WIN
#  define ITT_PLATFORM_WIN 1
#endif /* ITT_PLATFORM_WIN */ 

#ifndef ITT_PLATFORM_POSIX
#  define ITT_PLATFORM_POSIX 2
#endif /* ITT_PLATFORM_POSIX */

#ifndef ITT_PLATFORM
#  if ITT_OS==ITT_OS_WIN
#    define ITT_PLATFORM ITT_PLATFORM_WIN
#  else
#    define ITT_PLATFORM ITT_PLATFORM_POSIX
#  endif /* _WIN32 */
#endif /* ITT_PLATFORM */


#if ITT_PLATFORM==ITT_PLATFORM_WIN
#include <tchar.h>
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ITTAPI_CALL CDECL

#ifndef CDECL
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#    define CDECL __cdecl
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#    define CDECL
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* CDECL */

/** @endcond */

/** @brief user event type */
typedef int __itt_mark_type;
typedef int __itt_event;
typedef int __itt_state_t;

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#  ifdef UNICODE
     typedef wchar_t __itt_char;
#  else /* UNICODE */
     typedef char __itt_char;
#  endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @brief Typedef for char or wchar_t (if Unicode symbol is allowed) on Windows.
  * And typedef for char on Linux. 
  */
     typedef char __itt_char;
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @cond exclude_from_documentation */
typedef enum __itt_obj_state {
    __itt_obj_state_err = 0,
    __itt_obj_state_clr = 1,
    __itt_obj_state_set = 2,
    __itt_obj_state_use = 3
} __itt_obj_state_t;

typedef enum __itt_thr_state {
    __itt_thr_state_err = 0,
    __itt_thr_state_clr = 1,
    __itt_thr_state_set = 2
} __itt_thr_state_t;

typedef enum __itt_obj_prop {
    __itt_obj_prop_watch    = 1,
    __itt_obj_prop_ignore   = 2,
    __itt_obj_prop_sharable = 3
} __itt_obj_prop_t;

typedef enum __itt_thr_prop {
    __itt_thr_prop_quiet = 1
} __itt_thr_prop_t;
/** @endcond */
typedef enum __itt_error_code {
    __itt_error_success       = 0, /*!< no error                */
    __itt_error_no_module     = 1, /*!< module can't be loaded  */
    __itt_error_no_symbol     = 2, /*!< symbol not found        */
    __itt_error_unknown_group = 3, /*!< unknown group specified */
    __itt_error_cant_read_env = 4  /*!< variable value too long */
} __itt_error_code;

typedef void (__itt_error_notification_t)(__itt_error_code code, const char* msg);

/*******************************************
 * Various constants used by JIT functions *
 *******************************************/

 /*! @enum ___itt_jit_jvm_event
  * event notification 
  */
 typedef enum ___itt_jit_jvm_event
 {

   __itt_JVM_EVENT_TYPE_SHUTDOWN = 2,           /*!< Shutdown. Program exiting. EventSpecificData NA*/      
   __itt_JVM_EVENT_TYPE_METHOD_LOAD_FINISHED=13,/*!< JIT profiling. Issued after method code jitted into memory but before code is executed
												 *  event_data is an __itt_JIT_Method_Load */
   __itt_JVM_EVENT_TYPE_METHOD_UNLOAD_START     /*!< JIT profiling. Issued before unload. Method code will no longer be executed, but code and info are still in memory.
    											 *	The VTune profiler may capture method code only at this point. event_data is __itt_JIT_Method_Id */

 } __itt_jit_jvm_event;

/*! @enum ___itt_jit_environment_type
 * @brief Enumerator for the environment of methods 
 */
typedef enum ___itt_jit_environment_type
{
    __itt_JIT_JITTINGAPI = 2
} __itt_jit_environment_type;

/**********************************
 * Data structures for the events *
 **********************************/

 /*! @struct ___itt_jit_method_id
  * @brief structure for the events: __itt_iJVM_EVENT_TYPE_METHOD_UNLOAD_START    
  */
typedef struct ___itt_jit_method_id 
{
	/** @brief Id of the method (same as the one passed in the __itt_JIT_Method_Load struct */
    unsigned int       method_id;  

} *__itt_pjit_method_id, __itt_jit_method_id;

/*! @struct ___itt_jit_line_number_info
 *  @brief structure for the events: __itt_iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED 
 */
typedef struct ___itt_jit_line_number_info 
{
	/** @brief x86 Offset from the begining of the method */
    unsigned int        offset;    
	/** @brief source line number from the begining of the source file. */
    unsigned int        line_number;     

} *__itt_pjit_line_number_info, __itt_jit_line_number_info;
/*! @struct ___itt_jit_method_load
 *  @brief structure for the events: __itt_iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED 
 */
typedef struct ___itt_jit_method_load 
{
	/** @brief unique method ID - can be any unique value, (except 0 - 999) */
    unsigned int        method_id;
    /** @brief method name (can be with or without the class and signature, in any case the class name will be added to it) */
    char*               method_name;
    /** @brief virtual address of that method  - This determines the method range for the iJVM_EVENT_TYPE_ENTER/LEAVE_METHOD_ADDR events */
	void*               method_load_address;
    /** @brief Size in memory - Must be exact */
	unsigned int        method_size;
    /** @brief Line Table size in number of entries - Zero if none */
	unsigned int        line_number_size;
    /** @brief Pointer to the begining of the line numbers info array */
	__itt_pjit_line_number_info line_number_table;
    /** @brief unique class ID */
	unsigned int        class_id;
    /** @brief class file name */
	char*               class_file_name;
    /** @brief source file name */
	char*               source_file_name;
    /** @brief bits supplied by the user for saving in the JIT file... */
	void*               user_data;
    /** @brief the size of the user data buffer */
	unsigned int        user_data_size;
    /** @note no need to fill this field, it's filled by VTune */
	__itt_jit_environment_type env;
} *__itt_pjit_method_load, __itt_jit_method_load;

/** 
 * @brief General behavior: application continues to run, but no profiling information is being collected

 * - Pausing occurs not only for the current thread but for all process as well as spawned processes
 * - Intel(R) Parallel Inspector: does not analyze or report errors that involve memory access.
 * - Intel(R) Parallel Inspector: Other errors are reported as usual. Pausing data collection in
     Intel(R) Parallel Inspector only pauses tracing and analyzing memory access. It does not pause
     tracing or analyzing threading APIs.
 * - Intel(R) Parallel Amplifier: does continue to record when new threads are started
 * - Other effects: possible reduction of runtime overhead
 */
void ITTAPI_CALL __itt_pause(void);

/** 
 * @brief General behavior: application continues to run, collector resumes profiling information 
 * collection for all threads and processes of profiled application
 */
void ITTAPI_CALL __itt_resume(void);

#if ITT_PLATFORM==ITT_PLATFORM_WIN
__itt_mark_type ITTAPI_CALL __itt_mark_createA(const char *name);
__itt_mark_type ITTAPI_CALL __itt_mark_createW(const wchar_t *name);
#ifdef UNICODE
#  define __itt_mark_create __itt_mark_createW
#  define __itt_mark_create_ptr __itt_mark_createW_ptr
#else /* UNICODE */
#  define __itt_mark_create __itt_mark_createA
#  define __itt_mark_create_ptr __itt_mark_createA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @brief Creates a user event type (mark) with the specified name using char or Unicode string.
 * @param[in] name - name of mark to create
 * @return Returns a handle to the mark type
 */
__itt_mark_type ITTAPI_CALL __itt_mark_create(const __itt_char* name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#if ITT_PLATFORM==ITT_PLATFORM_WIN
int ITTAPI_CALL __itt_markA(__itt_mark_type mt, const char *parameter);
int ITTAPI_CALL __itt_markW(__itt_mark_type mt, const wchar_t *parameter);

int ITTAPI_CALL __itt_mark_globalA(__itt_mark_type mt, const char *parameter);
int ITTAPI_CALL __itt_mark_globalW(__itt_mark_type mt, const wchar_t *parameter);

#ifdef UNICODE
#  define __itt_mark __itt_markW
#  define __itt_mark_ptr __itt_markW_ptr

#  define __itt_mark_global __itt_mark_globalW
#  define __itt_mark_global_ptr __itt_mark_globalW_ptr
#else /* UNICODE  */
#  define __itt_mark __itt_markA
#  define __itt_mark_ptr __itt_markA_ptr

#  define __itt_mark_global __itt_mark_globalA
#  define __itt_mark_global_ptr __itt_mark_globalA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @brief Creates a "discrete" user event type (mark) of the specified type and an optional parameter using char or Unicode string.

 * - The mark of "discrete" type is placed to collection results in case of success. It appears in overtime view(s) as a special tick sign. 
 * - The call is "synchronous" - function returns after mark is actually added to results.
 * - This function is useful, for example, to mark different phases of application (beginning of the next mark automatically meand end of current region).
 * - Can be used together with "continuous" marks (see below) at the same collection session
 * @param[in] mt - mark, created by __itt_mark_create(const __itt_char* name) function
 * @param[in] parameter - string parameter of mark
 * @return Returns zero value in case of success, non-zero value otherwise.
 */
int ITTAPI_CALL __itt_mark(__itt_mark_type mt, const __itt_char* parameter);
/** @brief Use this if necessary to create a "discrete" user event type (mark) for process
 * rather then for one thread
 * @see int ITTAPI_CALL __itt_mark(__itt_mark_type mt, const __itt_char* parameter);
 */
int ITTAPI_CALL __itt_mark_global(__itt_mark_type mt, const __itt_char* parameter);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** 
 * @brief Creates an "end" point for "continuous" mark with specified name.

 * - Returns zero value in case of success, non-zero value otherwise. Also returns non-zero value when preceding "begin" point for the mark with the same name failed to be created or not created. (*)
 * - The mark of "continuous" type is placed to collection results in case of success. It appears in overtime view(s) as a special tick sign (different from "discrete" mark) together with line from corresponding "begin" mark to "end" mark. (*) * - Continuous marks can overlap (*) and be nested inside each other. Discrete mark can be nested inside marked region
 * 
 * @param[in] mt - mark, created by __itt_mark_create(const __itt_char* name) function
 * 
 * @return Returns zero value in case of success, non-zero value otherwise.
 */
int ITTAPI_CALL __itt_mark_off(__itt_mark_type mt);
/** @brief Use this if necessary to create an "end" point for mark of process
 * @see int ITTAPI_CALL __itt_mark_off(__itt_mark_type mt);
 */
int ITTAPI_CALL __itt_mark_global_off(__itt_mark_type mt);

#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI_CALL __itt_thread_set_nameA(const char *name);
void ITTAPI_CALL __itt_thread_set_nameW(const wchar_t *name);
#ifdef UNICODE
#  define __itt_thread_set_name __itt_thread_set_nameW
#  define __itt_thread_set_name_ptr __itt_thread_set_nameW_ptr
#else /* UNICODE */
#  define __itt_thread_set_name __itt_thread_set_nameA
#  define __itt_thread_set_name_ptr __itt_thread_set_nameA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @brief Sets thread name using char or Unicode string
 * @param[in] name - name of thread
 */
void ITTAPI_CALL __itt_thread_set_name(const __itt_char* name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @brief Mark current thread as ignored from this point on, for the duration of its existence. */
void ITTAPI_CALL __itt_thread_ignore(void);
/** @brief Is called when sync object is destroyed (needed to track lifetime of objects) */
void ITTAPI_CALL __itt_sync_destroy(void *addr);
/** @brief Enter spin loop on user-defined sync object */
void ITTAPI_CALL __itt_sync_prepare(void* addr);
/** @brief Quit spin loop without acquiring spin object */
void ITTAPI_CALL __itt_sync_cancel(void *addr);
/** @brief Successful spin loop completion (sync object acquired) */
void ITTAPI_CALL __itt_sync_acquired(void *addr);
/** @brief Start sync object releasing code. Is called before the lock release call. */
void ITTAPI_CALL __itt_sync_releasing(void* addr);
/** @brief Sync object released. Is called after the release call */
void ITTAPI_CALL __itt_sync_released(void* addr);

/** @brief Fast synchronization which does no require spinning.

  * - This special function is to be used by TBB and OpenMP libraries only when they know 
  *   there is no spin but they need to suppress TC warnings about shared variable modifications.
  * - It only has corresponding pointers in static library and does not have corresponding function
  *   in dynamic library.
  * @see void ITTAPI_CALL __itt_sync_prepare(void* addr);
*/
void ITTAPI_CALL __itt_fsync_prepare(void* addr);
/** @brief Fast synchronization which does no require spinning.

  * - This special function is to be used by TBB and OpenMP libraries only when they know 
  *   there is no spin but they need to suppress TC warnings about shared variable modifications.
  * - It only has corresponding pointers in static library and does not have corresponding function
  *   in dynamic library.
  * @see void ITTAPI_CALL __itt_sync_cancel(void *addr);
*/
void ITTAPI_CALL __itt_fsync_cancel(void *addr);
/** @brief Fast synchronization which does no require spinning.

  * - This special function is to be used by TBB and OpenMP libraries only when they know 
  *   there is no spin but they need to suppress TC warnings about shared variable modifications.
  * - It only has corresponding pointers in static library and does not have corresponding function
  *   in dynamic library.
  * @see void ITTAPI_CALL __itt_sync_acquired(void *addr);
*/
void ITTAPI_CALL __itt_fsync_acquired(void *addr);
/** @brief Fast synchronization which does no require spinning.

  * - This special function is to be used by TBB and OpenMP libraries only when they know 
  *   there is no spin but they need to suppress TC warnings about shared variable modifications.
  * - It only has corresponding pointers in static library and does not have corresponding function
  *   in dynamic library.
  * @see void ITTAPI_CALL __itt_sync_releasing(void* addr);
*/
void ITTAPI_CALL __itt_fsync_releasing(void* addr);
/** @brief Fast synchronization which does no require spinning.

  * - This special function is to be used by TBB and OpenMP libraries only when they know 
  *   there is no spin but they need to suppress TC warnings about shared variable modifications.
  * - It only has corresponding pointers in static library and does not have corresponding function
  *   in dynamic library.
  * @see void ITTAPI_CALL __itt_sync_released(void* addr);
*/
void ITTAPI_CALL __itt_fsync_released(void* addr);

/** @hideinitializer
 * @brief possible value of attribute argument for sync object type
 */
#define __itt_attr_barrier 1
/** @hideinitializer 
 * @brief possible value of attribute argument for sync object type
 */
#define __itt_attr_mutex   2

#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI_CALL __itt_sync_set_nameA(void *addr, const char *objtype, const char *objname, int attribute);
void ITTAPI_CALL __itt_sync_set_nameW(void *addr, const wchar_t *objtype, const wchar_t *objname, int attribute);
#ifdef UNICODE
#  define __itt_sync_set_name __itt_sync_set_nameW
#  define __itt_sync_set_name_ptr __itt_sync_set_nameW_ptr
#else /* UNICODE */
#  define __itt_sync_set_name __itt_sync_set_nameA
#  define __itt_sync_set_name_ptr __itt_sync_set_nameA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @deprecated Legacy API 
 * @brief Assign a name to a sync object using char or Unicode string
 *  @param[in] addr -    pointer to the sync object. You should use a real pointer to your object
 *                       to make sure that the values don't clash with other object addresses
 *  @param[in] objtype - null-terminated object type string. If NULL is passed, the object will
 *                       be assumed to be of generic "User Synchronization" type
 *  @param[in] objname - null-terminated object name string. If NULL, no name will be assigned
 *                       to the object -- you can use the __itt_sync_rename call later to assign
 *                       the name
 *  @param[in] attribute - one of [ #__itt_attr_barrier , #__itt_attr_mutex] values which defines the
 *                       exact semantics of how prepare/acquired/releasing calls work.
 */
void ITTAPI_CALL __itt_sync_set_name(void *addr, const __itt_char* objtype, const __itt_char* objname, int attribute);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */


#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI_CALL __itt_sync_createA(void *addr, const char *objtype, const char *objname, int attribute);
void ITTAPI_CALL __itt_sync_createW(void *addr, const wchar_t *objtype, const wchar_t *objname, int attribute);
#ifdef UNICODE
#define __itt_sync_create __itt_sync_createW
#  define __itt_sync_create_ptr __itt_sync_createW_ptr
#else /* UNICODE */
#define __itt_sync_create __itt_sync_createA
#  define __itt_sync_create_ptr __itt_sync_createA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @brief Register the creation of a sync object using char or Unicode string
 *  @param[in] addr -    pointer to the sync object. You should use a real pointer to your object
 *                       to make sure that the values don't clash with other object addresses
 *  @param[in] objtype - null-terminated object type string. If NULL is passed, the object will
 *                       be assumed to be of generic "User Synchronization" type
 *  @param[in] objname - null-terminated object name string. If NULL, no name will be assigned
 *                       to the object -- you can use the __itt_sync_rename call later to assign
 *                       the name
 *  @param[in] attribute - one of [ #__itt_attr_barrier, #__itt_attr_mutex] values which defines the
 *                       exact semantics of how prepare/acquired/releasing calls work.
**/
void ITTAPI_CALL __itt_sync_create(void *addr, const __itt_char* objtype, const __itt_char* objname, int attribute);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @brief Assign a name to a sync object using char or Unicode string.

 * Sometimes you cannot assign the name to a sync object in the __itt_sync_set_name call because it
 * is not yet known there. In this case you should use the rename call which allows to assign the
 * name after the creation has been registered. The renaming can be done multiple times. All waits
 * after a new name has been assigned will be attributed to the sync object with this name.
 * @param[in] addr -    pointer to the sync object
 * @param[in] name - null-terminated object name string
**/
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI_CALL __itt_sync_renameA(void *addr, const char *name);
void ITTAPI_CALL __itt_sync_renameW(void *addr, const wchar_t *name);
#ifdef UNICODE
#define __itt_sync_rename __itt_sync_renameW
#  define __itt_sync_rename_ptr __itt_sync_renameW_ptr
#else /* UNICODE */
#define __itt_sync_rename __itt_sync_renameA
#  define __itt_sync_rename_ptr __itt_sync_renameA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI_CALL __itt_sync_rename(void *addr, const __itt_char* name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentaion */
int __itt_jit_notify_event(__itt_jit_jvm_event event_type, void* event_data);
unsigned int __itt_jit_get_new_method_id(void);
const char* ITTAPI_CALL __itt_api_version(void);
__itt_error_notification_t* __itt_set_error_handler(__itt_error_notification_t*);

#if ITT_OS == ITT_OS_WIN
#define LIBITTNOTIFY_CC __cdecl
#define LIBITTNOTIFY_EXPORT __declspec(dllexport) 
#define LIBITTNOTIFY_IMPORT __declspec(dllimport) 
#elif ITT_OS == ITT_OS_MAC || ITT_OS == ITT_OS_LINUX
#define LIBITTNOTIFY_CC /* nothing */
#define LIBITTNOTIFY_EXPORT /* nothing */
#define LIBITTNOTIFY_IMPORT /* nothing */
#else /* ITT_OS == ITT_OS_WIN */
#error "Unsupported OS"
#endif /* ITT_OS == ITT_OS_WIN */

#define LIBITTNOTIFY_API
/** @endcond */

/** @deprecated Legacy API
 * @brief Hand instrumentation of user synchronization 
 */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_notify_sync_prepare(void *p);
/** @deprecated Legacy API
 * @brief Hand instrumentation of user synchronization 
 */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_notify_sync_cancel(void *p);
/** @deprecated Legacy API
 * @brief Hand instrumentation of user synchronization 
 */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_notify_sync_acquired(void *p);
/** @deprecated Legacy API
 * @brief Hand instrumentation of user synchronization 
 */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_notify_sync_releasing(void *p);
/** @deprecated Legacy API
 * @brief itt_notify_cpath_target is handled by Thread Profiler only.
 *  Inform Thread Profiler that the current thread has recahed a critical path target. 
 */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_notify_cpath_target(void);

#if ITT_PLATFORM==ITT_PLATFORM_WIN
LIBITTNOTIFY_API int LIBITTNOTIFY_CC __itt_thr_name_setA( char *name, int namelen );
LIBITTNOTIFY_API int LIBITTNOTIFY_CC __itt_thr_name_setW( wchar_t *name, int namelen );
# ifdef UNICODE
#  define __itt_thr_name_set __itt_thr_name_setW
#  define __itt_thr_name_set_ptr __itt_thr_name_setW_ptr
# else
#  define __itt_thr_name_set __itt_thr_name_setA
#  define __itt_thr_name_set_ptr __itt_thr_name_setA_ptr
# endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @deprecated Legacy API
 * @brief Set name to be associated with thread in analysis GUI.
 *  Return __itt_err upon failure (name or namelen being null,name and namelen mismatched)
 */
LIBITTNOTIFY_API int LIBITTNOTIFY_CC __itt_thr_name_set( __itt_char *name, int namelen );
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @brief Mark current thread as ignored from this point on, for the duration of its existence. */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC  __itt_thr_ignore(void);

/* User event notification                                                                    */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
/** @deprecated Legacy API 
 * @brief User event notification.
 * Event create APIs return non-zero event identifier upon success and __itt_err otherwise
 * (name or namelen being null/name and namelen not matching, user event feature not enabled)
 */
LIBITTNOTIFY_API __itt_event LIBITTNOTIFY_CC __itt_event_createA( char *name, int namelen );
LIBITTNOTIFY_API __itt_event LIBITTNOTIFY_CC __itt_event_createW( wchar_t *name, int namelen );
# ifdef UNICODE
#  define __itt_event_create __itt_event_createW
#  define __itt_event_create_ptr __itt_event_createW_ptr
# else
#  define __itt_event_create __itt_event_createA
#  define __itt_event_create_ptr __itt_event_createA_ptr
# endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
/** @deprecated Legacy API 
 * @brief User event notification.
 * Event create APIs return non-zero event identifier upon success and __itt_err otherwise
 * (name or namelen being null/name and namelen not matching, user event feature not enabled)
 */
LIBITTNOTIFY_API __itt_event LIBITTNOTIFY_CC __itt_event_create( __itt_char *name, int namelen );
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @deprecated Legacy API 
 * @brief Record an event occurance. 
 * These APIs return __itt_err upon failure (invalid event id/user event feature not enabled) 
 */
LIBITTNOTIFY_API int LIBITTNOTIFY_CC __itt_event_start( __itt_event event );
/** @deprecated Legacy API 
 * @brief Record an event occurance.  event_end is optional if events do not have durations. 
 * These APIs return __itt_err upon failure (invalid event id/user event feature not enabled) 
 */
LIBITTNOTIFY_API int LIBITTNOTIFY_CC __itt_event_end( __itt_event event ); /** optional */


/** @deprecated Legacy API 
 * @brief managing thread and object states
 */
LIBITTNOTIFY_API __itt_state_t LIBITTNOTIFY_CC __itt_state_get(void);
/** @deprecated Legacy API 
 * @brief managing thread and object states
 */
LIBITTNOTIFY_API __itt_state_t LIBITTNOTIFY_CC __itt_state_set( __itt_state_t );

/** @deprecated Legacy API 
 * @brief managing thread and object modes
 */
LIBITTNOTIFY_API __itt_thr_state_t LIBITTNOTIFY_CC __itt_thr_mode_set( __itt_thr_prop_t, __itt_thr_state_t );
/** @deprecated Legacy API 
 * @brief managing thread and object modes
 */
LIBITTNOTIFY_API __itt_obj_state_t LIBITTNOTIFY_CC __itt_obj_mode_set( __itt_obj_prop_t, __itt_obj_state_t );

/** @deprecated Non-supported Legacy API
 * @brief Inform the tool of memory accesses on reading
 */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_memory_read( void *address, size_t size );
/** @deprecated Non-supported Legacy API
 * @brief Inform the tool of memory accesses on writing
 */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_memory_write( void *address, size_t size );
/** @deprecated Non-supported Legacy API
 * @brief Inform the tool of memory accesses on updating
 */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_memory_update( void *address, size_t size );

/** @cond exclude_from_documentation */
/* The following 3 are currently for INTERNAL use only */
/** @internal */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_test_delay( int );
/** @internal */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_test_seq_init( void *, int );
/** @internal */
LIBITTNOTIFY_API void LIBITTNOTIFY_CC __itt_test_seq_wait( void *, int );
/** @endcond */

#ifdef __cplusplus
}
#endif /* __cplusplus */


/* *********************************************************************************
   *********************************************************************************
   ********************************************************************************* */
/** @cond exclude_from_documentation */
#define ITT_JOIN_AUX(p,n) p##n
#define ITT_JOIN(p,n) ITT_JOIN_AUX(p,n)

#ifndef INTEL_ITTNOTIFY_PREFIX
#define INTEL_ITTNOTIFY_PREFIX __itt_
#endif /* INTEL_ITTNOTIFY_PREFIX */
#ifndef INTEL_ITTNOTIFY_POSTFIX
#  define INTEL_ITTNOTIFY_POSTFIX _ptr_
#endif /* INTEL_ITTNOTIFY_POSTFIX */

#ifndef _ITTNOTIFY_H_MACRO_BODY_

#define ____ITTNOTIFY_NAME_(p,n) p##n
#define ___ITTNOTIFY_NAME_(p,n) ____ITTNOTIFY_NAME_(p,n)
#define __ITTNOTIFY_NAME_(n) ___ITTNOTIFY_NAME_(INTEL_ITTNOTIFY_PREFIX,n)
#define _ITTNOTIFY_NAME_(n) __ITTNOTIFY_NAME_(ITT_JOIN(n,INTEL_ITTNOTIFY_POSTFIX))

#ifdef ITT_STUBV
#undef ITT_STUBV
#endif
#define ITT_STUBV(type,name,args,params)                     \
    typedef type (ITTAPI_CALL* ITT_JOIN(_ITTNOTIFY_NAME_(name),_t)) args; \
    extern ITT_JOIN(_ITTNOTIFY_NAME_(name),_t) _ITTNOTIFY_NAME_(name);
#undef ITT_STUB
#define ITT_STUB ITT_STUBV

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define __itt_error_handler ITT_JOIN(INTEL_ITTNOTIFY_PREFIX, error_handler)
void __itt_error_handler(__itt_jit_jvm_event event_type, void* event_data);

extern const __itt_state_t _ITTNOTIFY_NAME_(state_err);
extern const __itt_event   _ITTNOTIFY_NAME_(event_err);
extern const int           _ITTNOTIFY_NAME_(err);

#define __itt_state_err _ITTNOTIFY_NAME_(state_err)
#define __itt_event_err _ITTNOTIFY_NAME_(event_err)
#define __itt_err       _ITTNOTIFY_NAME_(err)

ITT_STUBV(void, pause,(void),())
ITT_STUBV(void, resume,(void),())

#if ITT_PLATFORM==ITT_PLATFORM_WIN

ITT_STUB(__itt_mark_type, mark_createA,(const char *name),(name))

ITT_STUB(__itt_mark_type, mark_createW,(const wchar_t *name),(name))

ITT_STUB(int, markA,(__itt_mark_type mt, const char *parameter),(mt,parameter))

ITT_STUB(int, markW,(__itt_mark_type mt, const wchar_t *parameter),(mt,parameter))

ITT_STUB(int, mark_globalA,(__itt_mark_type mt, const char *parameter),(mt,parameter))

ITT_STUB(int, mark_globalW,(__itt_mark_type mt, const wchar_t *parameter),(mt,parameter))

ITT_STUBV(void, thread_set_nameA,( const char *name),(name))

ITT_STUBV(void, thread_set_nameW,( const wchar_t *name),(name))

ITT_STUBV(void, sync_createA,(void *addr, const char *objtype, const char *objname, int attribute), (addr, objtype, objname, attribute))

ITT_STUBV(void, sync_createW,(void *addr, const wchar_t *objtype, const wchar_t *objname, int attribute), (addr, objtype, objname, attribute))

ITT_STUBV(void, sync_renameA, (void *addr, const char *name), (addr, name))

ITT_STUBV(void, sync_renameW, (void *addr, const wchar_t *name), (addr, name))
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */

ITT_STUB(__itt_mark_type, mark_create,(const char *name),(name))

ITT_STUB(int, mark,(__itt_mark_type mt, const char *parameter),(mt,parameter))

ITT_STUB(int, mark_global,(__itt_mark_type mt, const char *parameter),(mt,parameter))

ITT_STUBV(void, sync_set_name,(void *addr, const char *objtype, const char *objname, int attribute),(addr,objtype,objname,attribute))

ITT_STUBV(void, thread_set_name,( const char *name),(name))

ITT_STUBV(void, sync_create,(void *addr, const char *objtype, const char *objname, int attribute), (addr, objtype, objname, attribute))

ITT_STUBV(void, sync_rename, (void *addr, const char *name), (addr, name))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

ITT_STUB(int, mark_off,(__itt_mark_type mt),(mt))

ITT_STUB(int, mark_global_off,(__itt_mark_type mt),(mt))

ITT_STUBV(void, thread_ignore,(void),())

ITT_STUBV(void, sync_prepare,(void* addr),(addr))

ITT_STUBV(void, sync_cancel,(void *addr),(addr))

ITT_STUBV(void, sync_acquired,(void *addr),(addr))

ITT_STUBV(void, sync_releasing,(void* addr),(addr))

ITT_STUBV(void, sync_released,(void* addr),(addr))

ITT_STUBV(void, fsync_prepare,(void* addr),(addr))

ITT_STUBV(void, fsync_cancel,(void *addr),(addr))

ITT_STUBV(void, fsync_acquired,(void *addr),(addr))

ITT_STUBV(void, fsync_releasing,(void* addr),(addr))

ITT_STUBV(void, fsync_released,(void* addr),(addr))

ITT_STUBV(void, sync_destroy,(void *addr), (addr))

ITT_STUBV(void, notify_sync_prepare,(void *p),(p))

ITT_STUBV(void, notify_sync_cancel,(void *p),(p))

ITT_STUBV(void, notify_sync_acquired,(void *p),(p))

ITT_STUBV(void, notify_sync_releasing,(void *p),(p))

ITT_STUBV(void, notify_cpath_target,(),())

#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUBV(void, sync_set_nameA,(void *addr, const char *objtype, const char *objname, int attribute),(addr,objtype,objname,attribute))

ITT_STUBV(void, sync_set_nameW,(void *addr, const wchar_t *objtype, const wchar_t *objname, int attribute),(addr,objtype,objname,attribute))

ITT_STUB (int, thr_name_setA,( char *name, int namelen ),(name,namelen))

ITT_STUB (int, thr_name_setW,( wchar_t *name, int namelen ),(name,namelen))

ITT_STUB (__itt_event, event_createA,( char *name, int namelen ),(name,namelen))

ITT_STUB (__itt_event, event_createW,( wchar_t *name, int namelen ),(name,namelen))
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUB (int, thr_name_set,( char *name, int namelen ),(name,namelen))

ITT_STUB (__itt_event, event_create,( char *name, int namelen ),(name,namelen))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

ITT_STUBV(void, thr_ignore,(void),())

ITT_STUB (int, event_start,( __itt_event event ),(event))

ITT_STUB (int, event_end,( __itt_event event ),(event))

ITT_STUB (__itt_state_t, state_get, (), ())
ITT_STUB (__itt_state_t, state_set,( __itt_state_t state), (state))
ITT_STUB (__itt_obj_state_t, obj_mode_set, ( __itt_obj_prop_t prop, __itt_obj_state_t state), (prop, state))
ITT_STUB (__itt_thr_state_t, thr_mode_set, (__itt_thr_prop_t prop, __itt_thr_state_t state), (prop, state))

ITT_STUB(const char*, api_version,(void),())

ITT_STUB(int, jit_notify_event, (__itt_jit_jvm_event event_type, void* event_data), (event_type, event_data))
ITT_STUB(unsigned int, jit_get_new_method_id, (void), ())

ITT_STUBV(void, memory_read,( void *address, size_t size ), (address, size))
ITT_STUBV(void, memory_write,( void *address, size_t size ), (address, size))
ITT_STUBV(void, memory_update,( void *address, size_t size ), (address, size))

ITT_STUBV(void, test_delay, (int p1), (p1))
ITT_STUBV(void, test_seq_init, ( void* p1, int p2), (p1, p2))
ITT_STUBV(void, test_seq_wait, ( void* p1, int p2), (p1, p2))
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


#ifndef INTEL_NO_ITTNOTIFY_API

#define __ITTNOTIFY_VOID_CALL__(n) (!_ITTNOTIFY_NAME_(n)) ? (void)0 : _ITTNOTIFY_NAME_(n)
#define __ITTNOTIFY_DATA_CALL__(n) (!_ITTNOTIFY_NAME_(n)) ? 0 : _ITTNOTIFY_NAME_(n)

#define __itt_pause __ITTNOTIFY_VOID_CALL__(pause)
#define __itt_pause_ptr _ITTNOTIFY_NAME_(pause)

#define __itt_resume __ITTNOTIFY_VOID_CALL__(resume)
#define __itt_resume_ptr _ITTNOTIFY_NAME_(resume)

#if ITT_PLATFORM==ITT_PLATFORM_WIN

#define __itt_mark_createA __ITTNOTIFY_DATA_CALL__(mark_createA)
#define __itt_mark_createA_ptr _ITTNOTIFY_NAME_(mark_createA)

#define __itt_mark_createW __ITTNOTIFY_DATA_CALL__(mark_createW)
#define __itt_mark_createW_ptr _ITTNOTIFY_NAME_(mark_createW)

#define __itt_markA __ITTNOTIFY_DATA_CALL__(markA)
#define __itt_markA_ptr _ITTNOTIFY_NAME_(markA)

#define __itt_markW __ITTNOTIFY_DATA_CALL__(markW)
#define __itt_markW_ptr _ITTNOTIFY_NAME_(markW)

#define __itt_mark_globalA __ITTNOTIFY_DATA_CALL__(mark_globalA)
#define __itt_mark_globalA_ptr _ITTNOTIFY_NAME_(mark_globalA)

#define __itt_mark_globalW __ITTNOTIFY_DATA_CALL__(mark_globalW)
#define __itt_mark_globalW_ptr _ITTNOTIFY_NAME_(mark_globalW)

#define __itt_thread_set_nameA __ITTNOTIFY_VOID_CALL__(thread_set_nameA)
#define __itt_thread_set_nameA_ptr _ITTNOTIFY_NAME_(thread_set_nameA)

#define __itt_thread_set_nameW __ITTNOTIFY_VOID_CALL__(thread_set_nameW)
#define __itt_thread_set_nameW_ptr _ITTNOTIFY_NAME_(thread_set_nameW)

#define __itt_sync_createA __ITTNOTIFY_VOID_CALL__(sync_createA)
#define __itt_sync_createA_ptr _ITTNOTIFY_NAME_(sync_createA)

#define __itt_sync_createW __ITTNOTIFY_VOID_CALL__(sync_createW)
#define __itt_sync_createW_ptr _ITTNOTIFY_NAME_(sync_createW)

#define __itt_sync_renameA __ITTNOTIFY_VOID_CALL__(sync_renameA)
#define __itt_sync_renameA_ptr _ITTNOTIFY_NAME_(sync_renameA)

#define __itt_sync_renameW __ITTNOTIFY_VOID_CALL__(sync_renameW)
#define __itt_sync_renameW_ptr _ITTNOTIFY_NAME_(sync_renameW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#define __itt_mark_create __ITTNOTIFY_DATA_CALL__(mark_create)
#define __itt_mark_create_ptr _ITTNOTIFY_NAME_(mark_create)

#define __itt_mark __ITTNOTIFY_DATA_CALL__(mark)
#define __itt_mark_ptr _ITTNOTIFY_NAME_(mark)

#define __itt_mark_global __ITTNOTIFY_DATA_CALL__(mark_global)
#define __itt_mark_global_ptr _ITTNOTIFY_NAME_(mark_global)

#define __itt_sync_set_name __ITTNOTIFY_VOID_CALL__(sync_set_name)
#define __itt_sync_set_name_ptr _ITTNOTIFY_NAME_(sync_set_name)

#define __itt_thread_set_name __ITTNOTIFY_VOID_CALL__(thread_set_name)
#define __itt_thread_set_name_ptr _ITTNOTIFY_NAME_(thread_set_name)

#define __itt_sync_create __ITTNOTIFY_VOID_CALL__(sync_create)
#define __itt_sync_create_ptr _ITTNOTIFY_NAME_(sync_create)

#define __itt_sync_rename __ITTNOTIFY_VOID_CALL__(sync_rename)
#define __itt_sync_rename_ptr _ITTNOTIFY_NAME_(sync_rename)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#define __itt_mark_off __ITTNOTIFY_DATA_CALL__(mark_off)
#define __itt_mark_off_ptr _ITTNOTIFY_NAME_(mark_off)

#define __itt_thread_ignore __ITTNOTIFY_VOID_CALL__(thread_ignore)
#define __itt_thread_ignore_ptr _ITTNOTIFY_NAME_(thread_ignore)

#define __itt_sync_prepare __ITTNOTIFY_VOID_CALL__(sync_prepare)
#define __itt_sync_prepare_ptr _ITTNOTIFY_NAME_(sync_prepare)

#define __itt_sync_cancel __ITTNOTIFY_VOID_CALL__(sync_cancel)
#define __itt_sync_cancel_ptr _ITTNOTIFY_NAME_(sync_cancel)

#define __itt_sync_acquired __ITTNOTIFY_VOID_CALL__(sync_acquired)
#define __itt_sync_acquired_ptr _ITTNOTIFY_NAME_(sync_acquired)

#define __itt_sync_releasing __ITTNOTIFY_VOID_CALL__(sync_releasing)
#define __itt_sync_releasing_ptr _ITTNOTIFY_NAME_(sync_releasing)

#define __itt_sync_released __ITTNOTIFY_VOID_CALL__(sync_released)
#define __itt_sync_released_ptr _ITTNOTIFY_NAME_(sync_released)

#define __itt_fsync_prepare __ITTNOTIFY_VOID_CALL__(fsync_prepare)
#define __itt_fsync_prepare_ptr _ITTNOTIFY_NAME_(fsync_prepare)

#define __itt_fsync_cancel __ITTNOTIFY_VOID_CALL__(fsync_cancel)
#define __itt_fsync_cancel_ptr _ITTNOTIFY_NAME_(fsync_cancel)

#define __itt_fsync_acquired __ITTNOTIFY_VOID_CALL__(fsync_acquired)
#define __itt_fsync_acquired_ptr _ITTNOTIFY_NAME_(fsync_acquired)

#define __itt_fsync_releasing __ITTNOTIFY_VOID_CALL__(fsync_releasing)
#define __itt_fsync_releasing_ptr _ITTNOTIFY_NAME_(fsync_releasing)

#define __itt_fsync_released __ITTNOTIFY_VOID_CALL__(fsync_released)
#define __itt_fsync_released_ptr _ITTNOTIFY_NAME_(fsync_released)

#define __itt_sync_destroy __ITTNOTIFY_VOID_CALL__(sync_destroy)
#define __itt_sync_destroy_ptr _ITTNOTIFY_NAME_(sync_destroy)

#define __itt_notify_sync_prepare __ITTNOTIFY_VOID_CALL__(notify_sync_prepare)
#define __itt_notify_sync_prepare_ptr _ITTNOTIFY_NAME_(notify_sync_prepare)

#define __itt_notify_sync_cancel __ITTNOTIFY_VOID_CALL__(notify_sync_cancel)
#define __itt_notify_sync_cancel_ptr _ITTNOTIFY_NAME_(notify_sync_cancel)

#define __itt_notify_sync_acquired __ITTNOTIFY_VOID_CALL__(notify_sync_acquired)
#define __itt_notify_sync_acquired_ptr _ITTNOTIFY_NAME_(notify_sync_acquired)

#define __itt_notify_sync_releasing __ITTNOTIFY_VOID_CALL__(notify_sync_releasing)
#define __itt_notify_sync_releasing_ptr _ITTNOTIFY_NAME_(notify_sync_releasing)

#define __itt_notify_cpath_target __ITTNOTIFY_VOID_CALL__(notify_cpath_target)
#define __itt_notify_cpath_target_ptr _ITTNOTIFY_NAME_(notify_cpath_target)

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_sync_set_nameA __ITTNOTIFY_VOID_CALL__(sync_set_nameA)
#define __itt_sync_set_nameA_ptr _ITTNOTIFY_NAME_(sync_set_nameA)

#define __itt_sync_set_nameW __ITTNOTIFY_VOID_CALL__(sync_set_nameW)
#define __itt_sync_set_nameW_ptr _ITTNOTIFY_NAME_(sync_set_nameW)

#define __itt_thr_name_setA __ITTNOTIFY_DATA_CALL__(thr_name_setA)
#define __itt_thr_name_setA_ptr _ITTNOTIFY_NAME_(thr_name_setA)

#define __itt_thr_name_setW __ITTNOTIFY_DATA_CALL__(thr_name_setW)
#define __itt_thr_name_setW_ptr _ITTNOTIFY_NAME_(thr_name_setW)

#define __itt_event_createA __ITTNOTIFY_DATA_CALL__(event_createA)
#define __itt_event_createA_ptr _ITTNOTIFY_NAME_(event_createA)

#define __itt_event_createW __ITTNOTIFY_DATA_CALL__(event_createW)
#define __itt_event_createW_ptr _ITTNOTIFY_NAME_(event_createW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_thr_name_set __ITTNOTIFY_DATA_CALL__(thr_name_set)
#define __itt_thr_name_set_ptr _ITTNOTIFY_NAME_(thr_name_set)

#define __itt_event_create __ITTNOTIFY_DATA_CALL__(event_create)
#define __itt_event_create_ptr _ITTNOTIFY_NAME_(event_create)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#define __itt_thr_ignore __ITTNOTIFY_VOID_CALL__(thr_ignore)
#define __itt_thr_ignore_ptr _ITTNOTIFY_NAME_(thr_ignore)

#define __itt_event_start __ITTNOTIFY_DATA_CALL__(event_start)
#define __itt_event_start_ptr _ITTNOTIFY_NAME_(event_start)

#define __itt_event_end __ITTNOTIFY_DATA_CALL__(event_end)
#define __itt_event_end_ptr _ITTNOTIFY_NAME_(event_end)

#define __itt_state_get __ITTNOTIFY_DATA_CALL__(state_get)
#define __itt_state_get_ptr _ITTNOTIFY_NAME_(state_get)

#define __itt_state_set __ITTNOTIFY_DATA_CALL__(state_set)
#define __itt_state_set_ptr _ITTNOTIFY_NAME_(state_set)

#define __itt_obj_mode_set __ITTNOTIFY_DATA_CALL__(obj_mode_set)
#define __itt_obj_mode_set_ptr _ITTNOTIFY_NAME_(obj_mode_set)

#define __itt_thr_mode_set __ITTNOTIFY_DATA_CALL__(thr_mode_set)
#define __itt_thr_mode_set_ptr _ITTNOTIFY_NAME_(thr_mode_set)

#define __itt_api_version __ITTNOTIFY_DATA_CALL__(api_version)
#define __itt_api_version_ptr _ITTNOTIFY_NAME_(api_version)

#define __itt_jit_notify_event __ITTNOTIFY_DATA_CALL__(jit_notify_event)
#define __itt_jit_notify_event_ptr _ITTNOTIFY_NAME_(jit_notify_event)

#define __itt_jit_get_new_method_id __ITTNOTIFY_DATA_CALL__(jit_get_new_method_id)
#define __itt_jit_get_new_method_id_ptr _ITTNOTIFY_NAME_(jit_get_new_method_id)

#define __itt_memory_read __ITTNOTIFY_VOID_CALL__(memory_read)
#define __itt_memory_read_ptr _ITTNOTIFY_NAME_(memory_read)

#define __itt_memory_write __ITTNOTIFY_VOID_CALL__(memory_write)
#define __itt_memory_write_ptr _ITTNOTIFY_NAME_(memory_write)

#define __itt_memory_update __ITTNOTIFY_VOID_CALL__(memory_update)
#define __itt_memory_update_ptr _ITTNOTIFY_NAME_(memory_update)


#define __itt_test_delay __ITTNOTIFY_VOID_CALL__(test_delay)
#define __itt_test_delay_ptr _ITTNOTIFY_NAME_(test_delay)

#define __itt_test_seq_init __ITTNOTIFY_VOID_CALL__(test_seq_init)
#define __itt_test_seq_init_ptr _ITTNOTIFY_NAME_(test_seq_init)

#define __itt_test_seq_wait __ITTNOTIFY_VOID_CALL__(test_seq_wait)
#define __itt_test_seq_wait_ptr _ITTNOTIFY_NAME_(test_seq_wait)

#define __itt_set_error_handler ITT_JOIN(INTEL_ITTNOTIFY_PREFIX, set_error_handler)

#else /* INTEL_NO_ITTNOTIFY_API */

#define __itt_pause()
#define __itt_pause_ptr 0

#define __itt_resume()
#define __itt_resume_ptr 0

#if ITT_PLATFORM==ITT_PLATFORM_WIN

#define __itt_mark_createA(name) (__itt_mark_type)0
#define __itt_mark_createA_ptr 0

#define __itt_mark_createW(name) (__itt_mark_type)0
#define __itt_mark_createW_ptr 0

#define __itt_markA(mt,parameter) (int)0
#define __itt_markA_ptr 0

#define __itt_markW(mt,parameter) (int)0
#define __itt_markW_ptr 0

#define __itt_mark_globalA(mt,parameter) (int)0
#define __itt_mark_globalA_ptr 0

#define __itt_mark_globalW(mt,parameter) (int)0
#define __itt_mark_globalW_ptr 0

#define __itt_thread_set_nameA(name)
#define __itt_thread_set_nameA_ptr 0

#define __itt_thread_set_nameW(name)
#define __itt_thread_set_nameW_ptr 0

#define __itt_sync_createA(addr, objtype, objname, attribute)
#define __itt_sync_createA_ptr 0

#define __itt_sync_createW(addr, objtype, objname, attribute)
#define __itt_sync_createW_ptr 0

#define __itt_sync_renameA(addr, name)
#define __itt_sync_renameA_ptr 0

#define __itt_sync_renameW(addr, name)
#define __itt_sync_renameW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#define __itt_mark_create(name) (__itt_mark_type)0
#define __itt_mark_create_ptr 0

#define __itt_mark(mt,parameter) (int)0
#define __itt_mark_ptr 0

#define __itt_mark_global(mt,parameter) (int)0
#define __itt_mark_global_ptr 0

#define __itt_sync_set_name(addr,objtype,objname,attribute)
#define __itt_sync_set_name_ptr 0

#define __itt_thread_set_name(name)
#define __itt_thread_set_name_ptr 0

#define __itt_sync_create(addr, objtype, objname, attribute)
#define __itt_sync_create_ptr 0

#define __itt_sync_rename(addr, name)
#define __itt_sync_rename_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#define __itt_mark_off(mt) (int)0
#define __itt_mark_off_ptr 0

#define __itt_thread_ignore()
#define __itt_thread_ignore_ptr 0

#define __itt_sync_prepare(addr)
#define __itt_sync_prepare_ptr 0

#define __itt_sync_cancel(addr)
#define __itt_sync_cancel_ptr 0

#define __itt_sync_acquired(addr)
#define __itt_sync_acquired_ptr 0

#define __itt_sync_releasing(addr)
#define __itt_sync_releasing_ptr 0

#define __itt_sync_released(addr)
#define __itt_sync_released_ptr 0

#define __itt_fsync_prepare(addr)
#define __itt_fsync_prepare_ptr 0

#define __itt_fsync_cancel(addr)
#define __itt_fsync_cancel_ptr 0

#define __itt_fsync_acquired(addr)
#define __itt_fsync_acquired_ptr 0

#define __itt_fsync_releasing(addr)
#define __itt_fsync_releasing_ptr 0

#define __itt_fsync_released(addr)
#define __itt_fsync_released_ptr 0

#define __itt_sync_destroy(addr)
#define __itt_sync_destroy_ptr 0

#define __itt_notify_sync_prepare(p)
#define __itt_notify_sync_prepare_ptr 0

#define __itt_notify_sync_cancel(p)
#define __itt_notify_sync_cancel_ptr 0

#define __itt_notify_sync_acquired(p)
#define __itt_notify_sync_acquired_ptr 0

#define __itt_notify_sync_releasing(p)
#define __itt_notify_sync_releasing_ptr 0

#define __itt_notify_cpath_target()
#define __itt_notify_cpath_target_ptr 0

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_sync_set_nameA(addr,objtype,objname,attribute)
#define __itt_sync_set_nameA_ptr 0

#define __itt_sync_set_nameW(addr,objtype,objname,attribute)
#define __itt_sync_set_nameW_ptr 0

#define __itt_thr_name_setA(name,namelen) (int)0
#define __itt_thr_name_setA_ptr 0

#define __itt_thr_name_setW(name,namelen) (int)0
#define __itt_thr_name_setW_ptr 0

#define __itt_event_createA(name,namelen) (__itt_event)0
#define __itt_event_createA_ptr 0

#define __itt_event_createW(name,namelen) (__itt_event)0
#define __itt_event_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_thr_name_set(name,namelen) (int)0
#define __itt_thr_name_set_ptr 0

#define __itt_event_create(name,namelen) (__itt_event)0
#define __itt_event_create_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#define __itt_thr_ignore()
#define __itt_thr_ignore_ptr 0

#define __itt_event_start(event) (int)0
#define __itt_event_start_ptr 0

#define __itt_event_end(event) (int)0
#define __itt_event_end_ptr 0

#define __itt_state_get() (__itt_state_t)0
#define __itt_state_get_ptr 0

#define __itt_state_set(state) (__itt_state_t)0
#define __itt_state_set_ptr 0

#define __itt_obj_mode_set(prop, state) (__itt_obj_state_t)0
#define __itt_obj_mode_set_ptr 0

#define __itt_thr_mode_set(prop, state) (__itt_thr_state_t)0
#define __itt_thr_mode_set_ptr 0

#define __itt_api_version() (const char*)0
#define __itt_api_version_ptr 0

#define __itt_jit_notify_event(event_type,event_data) (int)0
#define __itt_jit_notify_event_ptr 0

#define __itt_jit_get_new_method_id() (unsigned int)0
#define __itt_jit_get_new_method_id_ptr 0

#define __itt_memory_read(address, size)
#define __itt_memory_read_ptr 0

#define __itt_memory_write(address, size)
#define __itt_memory_write_ptr 0

#define __itt_memory_update(address, size)
#define __itt_memory_update_ptr 0

#define __itt_test_delay(p1)
#define __itt_test_delay_ptr 0

#define __itt_test_seq_init(p1,p2)
#define __itt_test_seq_init_ptr 0

#define __itt_test_seq_wait(p1,p2)
#define __itt_test_seq_wait_ptr 0

#define __itt_set_error_handler(x)

#endif /* INTEL_NO_ITTNOTIFY_API */

#endif /* _ITTNOTIFY_H_MACRO_BODY_ */

#endif /* _ITTNOTIFY_H_ */
/** @endcond */

