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

# Glob only and not recurse, there are other libs for that
file(GLOB_RECURSE shared_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.h)

# Exclude Win32 files
if(NOT WIN32)
  list(REMOVE_ITEM shared_SRCS
    WheatyExceptionReport.cpp
    WheatyExceptionReport.h
    ServiceWin32.cpp
    ServiceWin32.h
  )
endif()

source_group("Util"
  REGULAR_EXPRESSION .*
)

foreach(SRC ${shared_SRCS})
  get_filename_component(PTH ${SRC} PATH)
  if(PTH)
    if(NOT XCODE) # FIXME: Xcode Generator has bug with nested dirs
      string(REPLACE "/" "\\\\" PTH ${PTH})
    endif()
    source_group(${PTH} FILES ${SRC})
  endif()
endforeach(SRC)

source_group("DataStores"
  REGULAR_EXPRESSION DBC
)

source_group("Log"
  REGULAR_EXPRESSION Log
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/dep/include
  ${CMAKE_SOURCE_DIR}/src/framework
  ${CMAKE_BINARY_DIR}
  ${ACE_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SystemConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/SystemConfig.h)

add_library(shared STATIC
  ${shared_SRCS}
)

add_dependencies(shared revision.h)
if(NOT ACE_USE_EXTERNAL)
  add_dependencies(shared ACE_Project)
# add_dependencies(shared ace)
endif()
