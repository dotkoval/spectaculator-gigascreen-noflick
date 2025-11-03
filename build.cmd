@rem echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

set SRC=src\Main_Gigascreen.cpp src\lutmgr.cpp
cl /MD /LD /O2 /EHsc /DWIN32 /D_WINDOWS /DRENDERPLUGS_EXPORTS %SRC% /link /OUT:Gigascreen.rpi
