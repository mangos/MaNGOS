# Copyright (C) 2005-2013 MaNGOS project <http://getmangos.com/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

cmake_minimum_required (VERSION 2.6)
project (MANGOS_VMAP_EXTRACT_IO)

# All .cpp files are part of the executable...
file(GLOB src_files *.cpp)
add_executable(vmapextractor ${src_files})

# Needed for the StormLIb headers
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../../../dep/StormLib/src)

# Make sure the linker can find StormLib.
link_directories (${EXTRACT_SOURCE_DIR}/StormLib)

target_link_libraries(vmapextractor storm)

