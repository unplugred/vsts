@echo off
cls
SET _n=RedVJ
SET _m=null
:mode
SET /P _mode="[c]onfigure/[D]ebug/[r]elease/[m]in size rel/rel [w]ith deb info: " || SET _mode=d
If /I "%_mode%"=="c" goto configure
If /I "%_mode%"=="d" SET _m=Debug
If /I "%_mode%"=="r" SET _m=Release
If /I "%_mode%"=="m" SET _m=MinSizeRel
If /I "%_mode%"=="w" SET _m=RelWithDebInfo
If /I "%_m%"=="null" goto mode

:platform
SET _p=null
SET /P _platform="[a]ll build/[S]tandalone/[v]st/vst[3]: " || SET _platform=s
If /I "%_platform%"=="a" SET _p=ALL_BUILD
If /I "%_platform%"=="s" SET _p=%_n%_Standalone
If /I "%_platform%"=="v" SET _p=%_n%_VST
If /I "%_platform%"=="3" SET _p=%_n%_VST3
If /I "%_p%"=="null" goto platform

SET _r=n
If /I "%_platform%"=="a" SET _r=m
If /I "%_platform%"=="s" SET _r=m
If /I "%_r%"=="n" goto compile

:run
SET /P _r="run after compiling? ([Y]es/[n]o): " || SET _r=y
If /I "%_r%"=="y" goto compile
If /I "%_r%"=="n" goto compile
goto run

:compile
cmake --build build --config %_m% --target %_p%
If /I "%_r%"=="y" "./build/%_n%_artefacts/%_m%/Standalone/%_n%.exe"
goto end

:configure
cmake -B build -G "Visual Studio 17 2022"
goto end

:end
