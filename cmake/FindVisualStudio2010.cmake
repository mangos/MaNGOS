# This module defines
# VS_DEVENV, path to devenv.com.

FIND_PATH(VS100_DIR devenv.com
  $ENV{VS100COMNTOOLS}/../IDE
  "C:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\Common7\\IDE"
  "C:\\Program Files\\Microsoft Visual Studio 10.0\\Common7\\IDE"
  "C:\\Programme\\Microsoft Visual Studio 10.0\\Common7\\IDE"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\10.0\\Setup\\VS;EnvironmentDirectory]"
)

SET(VS100_FOUND 0)
IF(VS100_DIR)
  SET(VS100_FOUND 1)
  MESSAGE(STATUS "Found Visual Studion 2010")
ENDIF(VS100_DIR)

MARK_AS_ADVANCED(
  VS100_DIR
)
