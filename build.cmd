@rem echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

set SRC=src\Main_Gigascreen.cpp

set GAMMAS=25 24 22 20 18 linear

for %%G in (%GAMMAS%) do (
	cl /MD /LD /O2 /EHsc /DWIN32 /D_WINDOWS /DRENDERPLUGS_EXPORTS /DGAMMA=%%G %SRC% /link /OUT:Gigascreen_100_G%%G.rpi
	cl /MD /LD /O2 /EHsc /DWIN32 /D_WINDOWS /DRENDERPLUGS_EXPORTS /DGAMMA=%%G /DRETRO_VIBES %SRC% /link /OUT:Gigascreen_80_G%%G.rpi
)
