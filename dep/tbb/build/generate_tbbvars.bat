@echo off
REM
REM Copyright 2005-2009 Intel Corporation.  All Rights Reserved.
REM
REM This file is part of Threading Building Blocks.
REM
REM Threading Building Blocks is free software; you can redistribute it
REM and/or modify it under the terms of the GNU General Public License
REM version 2 as published by the Free Software Foundation.
REM
REM Threading Building Blocks is distributed in the hope that it will be
REM useful, but WITHOUT ANY WARRANTY; without even the implied warranty
REM of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with Threading Building Blocks; if not, write to the Free Software
REM Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
REM
REM As a special exception, you may use this file as part of a free software
REM library without restriction.  Specifically, if other files instantiate
REM templates or use macros or inline functions from this file, or you compile
REM this file and link it with other files to produce an executable, this
REM file does not by itself cause the resulting executable to be covered by
REM the GNU General Public License.  This exception does not however
REM invalidate any other reasons why the executable file might be covered by
REM the GNU General Public License.
REM
if exist tbbvars.bat exit
echo Generating tbbvars.bat
echo @echo off>tbbvars.bat
setlocal
for %%D in ("%tbb_root%") do set actual_root=%%~fD
if x%1==x goto without

echo SET TBB22_INSTALL_DIR=%actual_root%>>tbbvars.bat
echo SET TBB_ARCH_PLATFORM=%arch%\%runtime%>>tbbvars.bat
echo SET INCLUDE=%%TBB22_INSTALL_DIR%%\include;%%INCLUDE%%>>tbbvars.bat
echo SET LIB=%%TBB22_INSTALL_DIR%%\build\%1;%%LIB%%>>tbbvars.bat
echo SET PATH=%%TBB22_INSTALL_DIR%%\build\%1;%%PATH%%>>tbbvars.bat

if exist tbbvars.sh goto skipsh
set fslash_root=%actual_root:\=/%
echo Generating tbbvars.sh
echo #!/bin/sh>tbbvars.sh
echo export TBB22_INSTALL_DIR="%fslash_root%">>tbbvars.sh
echo TBB_ARCH_PLATFORM="%arch%\%runtime%">>tbbvars.sh
echo if [ -z "${PATH}" ]; then>>tbbvars.sh
echo     export PATH="${TBB22_INSTALL_DIR}/build/%1">>tbbvars.sh
echo else>>tbbvars.sh
echo     export PATH="${TBB22_INSTALL_DIR}/build/%1;$PATH">>tbbvars.sh
echo fi>>tbbvars.sh
echo if [ -z "${LIB}" ]; then>>tbbvars.sh
echo     export LIB="${TBB22_INSTALL_DIR}/build/%1">>tbbvars.sh
echo else>>tbbvars.sh
echo     export LIB="${TBB22_INSTALL_DIR}/build/%1;$LIB">>tbbvars.sh
echo fi>>tbbvars.sh
echo if [ -z "${INCLUDE}" ]; then>>tbbvars.sh
echo     export INCLUDE="${TBB22_INSTALL_DIR}/include">>tbbvars.sh
echo else>>tbbvars.sh
echo     export INCLUDE="${TBB22_INSTALL_DIR}/include;$INCLUDE">>tbbvars.sh
echo fi>>tbbvars.sh
:skipsh

if exist tbbvars.csh goto skipcsh
echo Generating tbbvars.csh
echo #!/bin/csh>tbbvars.csh
echo setenv TBB22_INSTALL_DIR "%actual_root%">>tbbvars.csh
echo setenv TBB_ARCH_PLATFORM "%arch%\%runtime%">>tbbvars.csh
echo if (! $?PATH) then>>tbbvars.csh
echo     setenv PATH "${TBB22_INSTALL_DIR}\build\%1">>tbbvars.csh
echo else>>tbbvars.csh
echo     setenv PATH "${TBB22_INSTALL_DIR}\build\%1;$PATH">>tbbvars.csh
echo endif>>tbbvars.csh
echo if (! $?LIB) then>>tbbvars.csh
echo     setenv LIB "${TBB22_INSTALL_DIR}\build\%1">>tbbvars.csh
echo else>>tbbvars.csh
echo     setenv LIB "${TBB22_INSTALL_DIR}\build\%1;$LIB">>tbbvars.csh
echo endif>>tbbvars.csh
echo if (! $?INCLUDE) then>>tbbvars.csh
echo     setenv INCLUDE "${TBB22_INSTALL_DIR}\include">>tbbvars.csh
echo else>>tbbvars.csh
echo     setenv INCLUDE "${TBB22_INSTALL_DIR}\include;$INCLUDE">>tbbvars.csh
echo endif>>tbbvars.csh
)
:skipcsh
exit

:without
set bin_dir=%CD%
echo SET tbb_root=%actual_root%>>tbbvars.bat
echo SET tbb_bin=%bin_dir%>>tbbvars.bat
echo SET TBB_ARCH_PLATFORM=%arch%\%runtime%>>tbbvars.bat
echo SET INCLUDE="%%tbb_root%%\include";%%INCLUDE%%>>tbbvars.bat
echo SET LIB="%%tbb_bin%%";%%LIB%%>>tbbvars.bat
echo SET PATH="%%tbb_bin%%";%%PATH%%>>tbbvars.bat

endlocal
