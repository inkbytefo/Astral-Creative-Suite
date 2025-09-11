@echo off
echo Compiling Astral Creative Suite 2D Shaders...

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

:: Compile 2D vertex shaders
echo Compiling 2D vertex shaders...
"%GLSLC_PATH%" -fshader-stage=vertex shaders/2d_canvas.vert -o build/Debug/shaders/2d_canvas.vert.spv
"%GLSLC_PATH%" -fshader-stage=vertex shaders/2d_brush.vert -o build/Debug/shaders/2d_brush.vert.spv
"%GLSLC_PATH%" -fshader-stage=vertex shaders/2d_grid.vert -o build/Debug/shaders/2d_grid.vert.spv

:: Compile 2D fragment shaders
echo Compiling 2D fragment shaders...
"%GLSLC_PATH%" -fshader-stage=fragment shaders/2d_canvas.frag -o build/Debug/shaders/2d_canvas.frag.spv
"%GLSLC_PATH%" -fshader-stage=fragment shaders/2d_brush.frag -o build/Debug/shaders/2d_brush.frag.spv
"%GLSLC_PATH%" -fshader-stage=fragment shaders/2d_grid.frag -o build/Debug/shaders/2d_grid.frag.spv

:: Copy to Release directory
echo Copying to Release directory...
copy build\Debug\shaders\*.spv build\Release\shaders\

echo Shader compilation completed successfully!
echo.
echo Generated files in build/Debug/shaders/:
dir build\Debug\shaders\*.spv

pause