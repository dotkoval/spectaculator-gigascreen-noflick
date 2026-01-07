@rem echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

cl /MD /LD /O2 /EHsc /DWIN32 /D_WINDOWS /DNDEBUG /DRENDERPLUGS_EXPORTS ^
	src\gigascreen_main.cpp ^
	src\lut_manager.cpp ^
    src\config_manager.cpp ^
    src\notifications_manager.cpp ^
	/link /OUT:gigascreen.rpi user32.lib
