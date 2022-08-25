#!/bin/bash
vn=VU
vm=null
read -p "[c]onfigure/[D]ebug/[r]elease/[m]in size rel/rel [w]ith deb info: " -n 1 -r
REPLY=${REPLY:-D}
echo
if [[ $REPLY =~ ^[Cc]$ ]]
then
	cmake -B build -G "Unix Makefiles"
	exit 1
else
	if [[ $REPLY =~ ^[Dd]$ ]]
	then
		vm=Debug
	else
		if [[ $REPLY =~ ^[Rr]$ ]]
		then
			vm=Release
		else
			if [[ $REPLY =~ ^[Mm]$ ]]
			then
				vm=MinSizeRel
			else
				if [[ $REPLY =~ ^[Ww]$ ]]
				then
					vm=RelWithDebInfo
				else
					exit 1
				fi
			fi
		fi
	fi
fi

vp=null
vr=n
read -p "[a]ll build/[S]tandalone/[v]st/vst[3]: " -n 1 -r
REPLY=${REPLY:-S}
echo
if [[ $REPLY =~ ^[Aa]$ ]]
then
	vp=ALL_BUILD
	vr=m
else
	if [[ $REPLY =~ ^[Ss]$ ]]
	then
		vp=${vn}_Standalone
		vr=m
	else
		if [[ $REPLY =~ ^[Vv]$ ]]
		then
			vp=${vn}_VST
		else
			if [[ $REPLY =~ ^[3t]$ ]]
			then
				vp=${vn}_VST3
			else
				exit 1
			fi
		fi
	fi
fi

if [ $vr == m ]
then
	read -p "run after compiling? ([Y]es/[n]o): " -n 1 -r
	REPLY=${REPLY:-Y}
	echo
	if [[ $REPLY =~ ^[Yy]$ ]]
	then
		vr=y
	else
		if [[ $REPLY =~ ^[Nn]$ ]]
		then
			vr=n
		else
			exit 1
		fi
	fi
fi

cmake --build build --config ${vm} --target ${vp}
if [ $vr == y ]
then
	"./build/${vn}_artefacts/${vm}/Standalone/${vn}"
fi
