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

# file(GLOB g3dlite_SRCS *.cpp) # don't need all of them

set(g3dlite_SRCS
  AABox.cpp
	Box.cpp
	Crypto.cpp
	format.cpp
	Matrix3.cpp
	Plane.cpp
	System.cpp
	Triangle.cpp
	Vector3.cpp
	Vector4.cpp
	debugAssert.cpp
	fileutils.cpp
	g3dmath.cpp
	g3dfnmatch.cpp
	prompt.cpp
	stringutils.cpp
	Any.cpp
	BinaryFormat.cpp
	BinaryInput.cpp
	BinaryOutput.cpp
	Capsule.cpp
	CollisionDetection.cpp
	CoordinateFrame.cpp
	Cylinder.cpp
	Line.cpp
	LineSegment.cpp
	Log.cpp
	Matrix4.cpp
	MemoryManager.cpp
	Quat.cpp
	Random.cpp
	Ray.cpp
	ReferenceCount.cpp
	RegistryUtil.cpp
	Sphere.cpp
	TextInput.cpp
	TextOutput.cpp
	UprightFrame.cpp
	Vector2.cpp
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/dep/include
  ${CMAKE_SOURCE_DIR}/dep/include/g3dlite
  ${CMAKE_SOURCE_DIR}/dep/include/zlib
)

add_library(g3dlite STATIC
  ${g3dlite_SRCS}
)

if(WIN32)
  target_link_libraries(g3dlite
    zlib
  )
endif()
