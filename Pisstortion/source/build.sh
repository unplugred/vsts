#!/bin/bash
vs=Pisstortion
vn=Pisstortion
vm=null
read -p "[c]onfigure/[D]ebug/[r]elease/[m]in size rel/rel [w]ith deb info: " -n 1 -r
REPLY=${REPLY:-D}
echo
if [[ $REPLY =~ ^[Cc]$ ]]
then

	vb=null
	read -p "[f]ree/[P]aid/[b]eta: " -n 1 -r
	REPLY=${REPLY:-P}
	echo
	if [[ $REPLY =~ ^[Pp]$ ]]
	then
		vb=0
	else
		if [[ $REPLY =~ ^[Ff]$ ]]
		then
			vb=1
		else
			if [[ $REPLY =~ ^[Bb]$ ]]
			then
				vb=2
			else
				exit 1
			fi
		fi
	fi

	cmake -DBANNERTYPE=${vb} -B build_linux -G "Unix Makefiles"
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
read -p "[a]ll build/[S]tandalone/[v]st3/[c]lap: " -n 1 -r
REPLY=${REPLY:-S}
echo
if [[ $REPLY =~ ^[Aa]$ ]]
then
	vp=${vn}_All
	vr=m
else
	if [[ $REPLY =~ ^[Ss]$ ]]
	then
		vp=${vn}_Standalone
		vr=m
	else
		if [[ $REPLY =~ ^[Vv]$ ]]
		then
			vp=${vn}_VST3
		else
			if [[ $REPLY =~ ^[Cc]$ ]]
			then
				vp=${vn}_CLAP
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

cmake --build build_linux --config ${vm} --target ${vp}
if [ $vp == ${vn}_All ]
then
	cmake --build build_linux --config ${vm} --target ${vn}_CLAP
	cp "./build_linux/${vn}_artefacts/CLAP/${vs}.clap" "../../Setup/build_linux/paid/${vs}.clap"
else
	if [ $vp == ${vn}_CLAP ]
	then
		cp "./build_linux/${vn}_artefacts/CLAP/${vs}.clap" "../../Setup/build_linux/paid/${vs}.clap"
	fi
fi
if [ $vr == y ]
then
	"./build_linux/${vn}_artefacts/Standalone/${vs}"
fi
