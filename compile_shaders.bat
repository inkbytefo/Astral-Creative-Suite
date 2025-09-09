@echo off
echo Compiling Astral Engine Shaders...

set GLSLC_PATH=%VULKAN_SDK%\Bin\glslc.exe

if not exist "%GLSLC_PATH%" (
    echo ERROR: glslc not found at %GLSLC_PATH%
    echo Please install Vulkan SDK and set VULKAN_SDK environment variable
    pause
    exit /b 1
)

:: Create output directories
if not exist "build\Debug\shaders" mkdir build\Debug\shaders
if not exist "build\Release\shaders" mkdir build\Release\shaders

:: Compile vertex shaders
echo Compiling vertex shaders...
"%GLSLC_PATH%" -fshader-stage=vertex shaders/basic.vert -o build/Debug/shaders/basic.vert.spv
"%GLSLC_PATH%" -fshader-stage=vertex shaders/pbr.vert -o build/Debug/shaders/pbr.vert.spv
"%GLSLC_PATH%" -fshader-stage=vertex shaders/shadow_depth.vert -o build/Debug/shaders/shadow_depth.vert.spv
"%GLSLC_PATH%" -fshader-stage=vertex shaders/triangle.vert -o build/Debug/shaders/triangle.vert.spv
"%GLSLC_PATH%" -fshader-stage=vertex shaders/unified_pbr.vert -o build/Debug/shaders/unified_pbr.vert.spv

:: Compile fragment shaders
echo Compiling fragment shaders...
"%GLSLC_PATH%" -fshader-stage=fragment shaders/shadow_null.frag -o build/Debug/shaders/shadow_null.frag.spv
"%GLSLC_PATH%" -fshader-stage=fragment shaders/unified_pbr.frag -o build/Debug/shaders/unified_pbr.frag.spv

:: Copy to Release directory
echo Copying to Release directory...
copy build\Debug\shaders\*.spv build\Release\shaders\

echo Shader compilation completed successfully!
echo.
echo Generated files in build/Debug/shaders/:
dir build\Debug\shaders\*.spv

pause
