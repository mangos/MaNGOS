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
project (EXTRACT)

# We want stormlib!
add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/../../dep/StormLib
${CMAKE_CURRENT_BINARY_DIR}/StormLib)


add_subdirectory (vmapextract)
