#
# Copyright (C) 2005-2012 MaNGOS project <http://getmangos.com/>
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

if(WIN32)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ace/config.h.win ${CMAKE_CURRENT_SOURCE_DIR}/ace/config.h)
endif()

if(WIN32 AND MSVC)
  # VS100 uses MSBuild.exe instead of devenv.com, so force it to use devenv.com
  if(VS100_FOUND)
    set(ACE_BUILD_TOOL ${VS100_DIR}/devenv.com)
  else()
    set(ACE_BUILD_TOOL ${CMAKE_BUILD_TOOL})
  endif()

  if(PLATFORM MATCHES X86)
    set(ACE_CONFIGURATION Win32)
  else()
    set(ACE_CONFIGURATION x64)
  endif()

  ExternalProject_Add(ACE_Project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
  ExternalProject_Add_Step(ACE_Project ACE_Upgrade
    COMMAND ${ACE_BUILD_TOOL} <SOURCE_DIR>\\ace\\ace_vc8.sln /upgrade
    ALWAYS 0
  )
  ExternalProject_Add_Step(ACE_Project ACE_Build
    DEPENDEES ACE_Upgrade
    COMMAND ${ACE_BUILD_TOOL} <SOURCE_DIR>\\ace\\ace_vc8.sln /project ACE /build ${CMAKE_BUILD_TYPE}|${ACE_CONFIGURATION}
    ALWAYS 0
  )
elseif(UNIX)
  if(CMAKE_BUILD_TOOL MATCHES "/gmake")
    set(ACE_BUILD_TOOL gmake)
  else()
    set(ACE_BUILD_TOOL make)
  endif()

  ExternalProject_Add(ACE_Project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --disable-ssl
    BUILD_COMMAND ${ACE_BUILD_TOOL} -j2 # export arch="ia32" &&
    INSTALL_COMMAND ${ACE_BUILD_TOOL} install
  )
else()
  message(FATAL_ERROR
    "This script doesn't support your system configuration to compile/install ACE."
  )
endif()
