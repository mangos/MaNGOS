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

#ifndef ITT_STUB
#define ITT_STUB ITT_STUBV
#endif /* ITT_STUB */

#ifndef ITTAPI_CALL
#define ITTAPI_CALL CDECL
#endif /* ITTAPI_CALL */

/* parameters for macro:
   type, func_name, arguments, params, func_name_in_dll, group
   */

ITT_STUBV(void, pause,(void),(), pause, __itt_control_group)

ITT_STUBV(void, resume,(void),(), resume, __itt_control_group)

#if ITT_PLATFORM==ITT_PLATFORM_WIN

ITT_STUB(int, markA,(__itt_mark_type mt, const char *parameter),(mt,parameter), markA, __itt_mark_group)

ITT_STUB(int, markW,(__itt_mark_type mt, const wchar_t *parameter),(mt,parameter), markW, __itt_mark_group)

ITT_STUB(int, mark_globalA,(__itt_mark_type mt, const char *parameter),(mt,parameter), mark_globalA, __itt_mark_group)

ITT_STUB(int, mark_globalW,(__itt_mark_type mt, const wchar_t *parameter),(mt,parameter), mark_globalW, __itt_mark_group)

ITT_STUBV(void, thread_set_nameA,( const char *name),(name), thread_set_nameA, __itt_thread_group)

ITT_STUBV(void, thread_set_nameW,( const wchar_t *name),(name), thread_set_nameW, __itt_thread_group)

ITT_STUBV(void, sync_createA,(void *addr, const char *objtype, const char *objname, int attribute), (addr, objtype, objname, attribute), sync_createA, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, sync_createW,(void *addr, const wchar_t *objtype, const wchar_t *objname, int attribute), (addr, objtype, objname, attribute), sync_createW, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, sync_renameA, (void *addr, const char *name), (addr, name), sync_renameA, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, sync_renameW, (void *addr, const wchar_t *name), (addr, name), sync_renameW, __itt_sync_group | __itt_fsync_group)
#else /* WIN32 */

ITT_STUB(int, mark,(__itt_mark_type mt, const char *parameter),(mt,parameter), mark, __itt_mark_group)
ITT_STUB(int, mark_global,(__itt_mark_type mt, const char *parameter),(mt,parameter), mark_global, __itt_mark_group)

ITT_STUBV(void, sync_set_name,(void *addr, const char *objtype, const char *objname, int attribute),(addr,objtype,objname,attribute), sync_set_name, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, thread_set_name,( const char *name),(name), thread_set_name, __itt_thread_group)

ITT_STUBV(void, sync_create,(void *addr, const char *objtype, const char *objname, int attribute), (addr, objtype, objname, attribute), sync_create, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, sync_rename, (void *addr, const char *name), (addr, name), sync_rename, __itt_sync_group | __itt_fsync_group)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

ITT_STUBV(void, sync_destroy,(void *addr), (addr), sync_destroy, __itt_sync_group | __itt_fsync_group)

ITT_STUB(int, mark_off,(__itt_mark_type mt),(mt), mark_off, __itt_mark_group)
ITT_STUB(int, mark_global_off,(__itt_mark_type mt),(mt), mark_global_off, __itt_mark_group)

ITT_STUBV(void, thread_ignore,(void),(), thread_ignore, __itt_thread_group)

ITT_STUBV(void, sync_prepare,(void* addr),(addr), sync_prepare, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, sync_cancel,(void *addr),(addr), sync_cancel, __itt_sync_group)

ITT_STUBV(void, sync_acquired,(void *addr),(addr), sync_acquired, __itt_sync_group)

ITT_STUBV(void, sync_releasing,(void* addr),(addr), sync_releasing, __itt_sync_group)

ITT_STUBV(void, sync_released,(void* addr),(addr), sync_released, __itt_sync_group)

ITT_STUBV(void, memory_read,( void *address, size_t size ), (address, size), memory_read, __itt_all_group)
ITT_STUBV(void, memory_write,( void *address, size_t size ), (address, size), memory_write, __itt_all_group)
ITT_STUBV(void, memory_update,( void *address, size_t size ), (address, size), memory_update, __itt_all_group)

ITT_STUB(int, jit_notify_event,(__itt_jit_jvm_event event_type, void* event_data),(event_type, event_data), jit_notify_event, __itt_jit_group)

#ifndef NO_ITT_LEGACY

#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUB(__itt_mark_type, mark_createA,(const char *name),(name), mark_createA, __itt_mark_group)
ITT_STUB(__itt_mark_type, mark_createW,(const wchar_t *name),(name), mark_createW, __itt_mark_group)
#else /* WIN32 */
ITT_STUB(__itt_mark_type, mark_create,(const char *name),(name), mark_create, __itt_mark_group)
#endif
ITT_STUBV(void, fsync_prepare,(void* addr),(addr), sync_prepare, __itt_fsync_group)

ITT_STUBV(void, fsync_cancel,(void *addr),(addr), sync_cancel, __itt_fsync_group)

ITT_STUBV(void, fsync_acquired,(void *addr),(addr), sync_acquired, __itt_fsync_group)

ITT_STUBV(void, fsync_releasing,(void* addr),(addr), sync_releasing, __itt_fsync_group)

ITT_STUBV(void, fsync_released,(void* addr),(addr), sync_released, __itt_fsync_group)

ITT_STUBV(void, notify_sync_prepare,(void *p),(p), notify_sync_prepare, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, notify_sync_cancel,(void *p),(p), notify_sync_cancel, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, notify_sync_acquired,(void *p),(p), notify_sync_acquired, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, notify_sync_releasing,(void *p),(p), notify_sync_releasing, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, notify_cpath_target,(void),(), notify_cpath_target, __itt_all_group)

#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUBV(void, sync_set_nameA,(void *addr, const char *objtype, const char *objname, int attribute),(addr,objtype,objname,attribute), sync_set_nameA, __itt_sync_group | __itt_fsync_group)

ITT_STUBV(void, sync_set_nameW,(void *addr, const wchar_t *objtype, const wchar_t *objname, int attribute),(addr,objtype,objname,attribute), sync_set_nameW, __itt_sync_group | __itt_fsync_group)

ITT_STUB (int, thr_name_setA,( char *name, int namelen ),(name,namelen), thr_name_setA, __itt_thread_group)

ITT_STUB (int, thr_name_setW,( wchar_t *name, int namelen ),(name,namelen), thr_name_setW, __itt_thread_group)

ITT_STUB (__itt_event, event_createA,( char *name, int namelen ),(name,namelen), event_createA, __itt_mark_group)

ITT_STUB (__itt_event, event_createW,( wchar_t *name, int namelen ),(name,namelen), event_createW, __itt_mark_group)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUB (int, thr_name_set,( char *name, int namelen ),(name,namelen), thr_name_set, __itt_thread_group)

ITT_STUB (__itt_event, event_create,( char *name, int namelen ),(name,namelen), event_create, __itt_mark_group)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

ITT_STUBV(void, thr_ignore,(void),(), thr_ignore, __itt_thread_group)

ITT_STUB (int, event_start,( __itt_event event ),(event), event_start, __itt_mark_group)

ITT_STUB (int, event_end,( __itt_event event ),(event), event_end, __itt_mark_group)

ITT_STUB (__itt_state_t, state_get, (void), (), state_get, __itt_all_group)
ITT_STUB (__itt_state_t, state_set,( __itt_state_t state), (state), state_set, __itt_all_group)
ITT_STUB (__itt_obj_state_t, obj_mode_set, ( __itt_obj_prop_t prop, __itt_obj_state_t state), (prop, state), obj_mode_set, __itt_all_group)
ITT_STUB (__itt_thr_state_t, thr_mode_set, (__itt_thr_prop_t prop, __itt_thr_state_t state), (prop, state), thr_mode_set, __itt_all_group)

ITT_STUB (const char*, api_version,(void),(), api_version, __itt_all_group)
ITT_STUB (unsigned int, jit_get_new_method_id, (void), (), jit_get_new_method_id, __itt_jit_group)

#endif /* NO_ITT_LEGACY */

