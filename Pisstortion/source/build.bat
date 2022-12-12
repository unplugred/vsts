@echo off
cls
SET _n=Pisstortion
SET _m=null
:mode
SET /P _mode="[c]onfigure/[D]ebug/[r]elease/[m]in size rel/rel [w]ith deb info: " || SET _mode=d
If /I "%_mode%"=="c" goto banner
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
cmake --build build_windows --config %_m% --target %_p%
If /I "%_r%"=="y" "./build_windows/%_n%_artefacts/%_m%/Standalone/%_n%.exe"
goto end

:banner
SET _b=null
SET /P _banner="[f]ree/[P]aid/[b]eta: " || SET _banner=p
If /I "%_banner%"=="p" SET _b=0
If /I "%_banner%"=="f" SET _b=1
If /I "%_banner%"=="b" SET _b=2
If /I "%_b%"=="null" goto banner

:configure
cmake -DBANNERTYPE=%_b% -B build_windows -G "Visual Studio 17 2022" -T host=x64 -A x64
goto end

:end
