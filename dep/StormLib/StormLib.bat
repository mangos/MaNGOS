@echo off
rem Post-build batch for StormLib project
rem Called as StormLib.bat $(PlatformName) $(ConfigurationName)
rem Example: StormLib.bat x64 Debug

copy src\StormPort.h ..\aaa\inc
copy src\StormLib.h  ..\aaa\inc

if x%1 == xWin32 goto PlatformWin32
if x%1 == xx64 goto PlatformWin64
goto exit

:PlatformWin32
copy .\bin\Stormlib\%1\%2\*.lib    ..\aaa\lib32
goto exit

:PlatformWin64
copy .\bin\Stormlib\%1\%2\*.lib    ..\aaa\lib64
goto exit

:exit

