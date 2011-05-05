#
# Copyright (C) 2005-2011 MaNGOS project <http://getmangos.com/>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

include(ExternalProject)

if(WIN32 AND MSVC)
  # VS100 uses MSBuild.exe instead of devenv.com, so force it to use devenv.com
  if(VS100_FOUND)
    set(TBB_BUILD_TOOL ${VS100_DIR}/devenv.com)
    set(TBB_SOURCE <SOURCE_DIR>\\build\\vs100project)
  else()
    set(TBB_BUILD_TOOL ${CMAKE_BUILD_TOOL})
    set(TBB_SOURCE <SOURCE_DIR>\\build\\vsproject)
  endif()

  if(PLATFORM MATCHES X86)
    set(TBB_CONFIGURATION Win32)
  else()
    set(TBB_CONFIGURATION x64)
  endif()

  ExternalProject_Add(TBB_Project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
  ExternalProject_Add_Step(TBB_Project TBB_Upgrade
    COMMAND ${TBB_BUILD_TOOL} ${TBB_SOURCE}\\makefile.sln /upgrade
    ALWAYS 0
  )
  ExternalProject_Add_Step(TBB_Project TBB_Build
    DEPENDEES TBB_Upgrade
    COMMAND ${TBB_BUILD_TOOL} ${TBB_SOURCE}\\makefile.sln /build Debug|${TBB_CONFIGURATION}
    COMMAND ${TBB_BUILD_TOOL} ${TBB_SOURCE}\\makefile.sln /build Release|${TBB_CONFIGURATION}
    ALWAYS 0
  )
elseif(UNIX)
  if(APPLE)
    set(APPLE_BUILD cd <SOURCE_DIR>/build/libs_release && install_name_tool -id "<INSTALL_DIR>/lib/libtbb.dylib" libtbb.dylib && install_name_tool -id "<INSTALL_DIR>/lib/libtbbmalloc.dylib" libtbbmalloc.dylib && cd <SOURCE_DIR>/build/libs_debug && install_name_tool -id "<INSTALL_DIR>/lib/libtbb_debug.dylib" libtbb_debug.dylib && install_name_tool -id "<INSTALL_DIR>/lib/libtbbmalloc_debug.dylib" libtbbmalloc_debug.dylib)
  else()
    # Do nothing, but really 'nothing' makes infinite loops on Debian
    set(APPLE_BUILD cd .)
  endif()

  if(CMAKE_BUILD_TOOL MATCHES "/gmake")
    set(TBB_BUILD_TOOL gmake)
  else()
    set(TBB_BUILD_TOOL make)
  endif()

  ExternalProject_Add(TBB_Project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${TBB_BUILD_TOOL} -j2 -C <SOURCE_DIR> # export arch="ia32" &&
    INSTALL_COMMAND ${APPLE_BUILD}
  )
else()
  message(FATAL_ERROR
    "This script doesn't support your system configuration to compile/install TBB."
  )
endif()
