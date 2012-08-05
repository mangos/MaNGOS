@echo off
rem This BAT file updates the ZIP file that is to be uploaded to web
rem Only use when both 32-bit and 64-bit are properly compiled

echo Creating stormlib.zip ...
cd \Ladik\Appdir
zip.exe -ur9 ..\WWW\web\download\stormlib.zip StormLib\doc\*
zip.exe -ur9 ..\WWW\web\download\stormlib.zip StormLib\src\*
zip.exe -ur9 ..\WWW\web\download\stormlib.zip StormLib\storm_dll\*
zip.exe -ur9 ..\WWW\web\download\stormlib.zip StormLib\StormLib.xcodeproj\*
zip.exe -ur9 ..\WWW\web\download\stormlib.zip StormLib\stormlib_dll\*
zip.exe -ur9 ..\WWW\web\download\stormlib.zip StormLib\test\*
zip.exe -u9  ..\WWW\web\download\stormlib.zip StormLib\CMakeLists.txt
zip.exe -u9  ..\WWW\web\download\stormlib.zip StormLib\makefile.*
zip.exe -u9  ..\WWW\web\download\stormlib.zip StormLib\Info.plist
zip.exe -u9  ..\WWW\web\download\stormlib.zip StormLib\*.bat
zip.exe -u9  ..\WWW\web\download\stormlib.zip StormLib\*.sln
zip.exe -u9  ..\WWW\web\download\stormlib.zip StormLib\*.vcproj
echo.

echo Press any key to exit ...
pause >nul
