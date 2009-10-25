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

#ifndef __CONFIG_H_
#define __CONFIG_H_

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

#ifndef ITT_ARCH_IA32
#  define ITT_ARCH_IA32  1
#endif /* ITT_ARCH_IA32 */

#ifndef ITT_ARCH_IA32E
#  define ITT_ARCH_IA32E 2
#endif /* ITT_ARCH_IA32E */

#ifndef ITT_ARCH_IA64
#  define ITT_ARCH_IA64  3
#endif /* ITT_ARCH_IA64 */


#ifndef ITT_ARCH
#  if defined _M_X64 || defined _M_AMD64 || defined __x86_64__
#    define ITT_ARCH ITT_ARCH_IA32E
#  elif defined _M_IA64 || defined __ia64
#    define ITT_ARCH ITT_ARCH_IA64
#  else
#    define ITT_ARCH ITT_ARCH_IA32
#  endif
#endif

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

#endif /* __CONFIG_H_ */

