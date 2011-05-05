@ECHO off
mkdir build
cd build

if %1!==! goto without

cmake .. -G %1
if errorlevel 1 call cmake --help
goto end

:without
cmake ..

:end
cd ..