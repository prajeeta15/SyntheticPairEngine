@echo off
echo Setting up Visual Studio Build Environment...

:: Call the VS2019 build tools environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

echo.
echo Building Synthetic Pair Engine...

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with CMake
echo Configuring with CMake...
cmake -G "NMake Makefiles" ..

:: Build the project
echo Building project...
nmake

echo.
echo Build completed!

:: Run the demo
echo Running Synthetic Pair Engine Demo...
echo.
SyntheticPairEngine.exe

pause
