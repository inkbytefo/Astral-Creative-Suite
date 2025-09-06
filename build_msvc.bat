@echo off
echo AstralEngine Build Script for MSVC
echo ==================================

REM Temizle
echo Cleaning build directory...
if exist build rmdir /s /q build

REM Yeni build dizini oluştur
echo Creating build directory...
mkdir build
cd build

REM CMake'i çalıştır
echo Running CMake...
cmake .. -G "Visual Studio 17 2022"

REM Derle
echo Building project...
cmake --build . --config Release

echo.
echo Build completed! Check for any errors above.
pause