#!/bin/bash
vn=VU
vs=VU
if [[ "$OSTYPE" =~ ^msys ]]
then
	vf=build_windows
else
	if [[ "$OSTYPE" =~ ^darwin ]]
	then
		vf=build_mac
	else
		vf=build_linux
	fi
fi

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
				read -p "done."
				exit 1
			fi
		fi
	fi

	if [[ "$OSTYPE" =~ ^msys ]]
	then
		cmake -DBANNERTYPE=${vb} -B ${vf} -G "Visual Studio 17 2022" -T host=x64 -A x64
	else
		if [[ "$OSTYPE" =~ ^darwin ]]
		then
			cmake -DBANNERTYPE=${vb} -B ${vf} -G "Xcode"
		else
			cmake -DBANNERTYPE=${vb} -B ${vf} -G "Unix Makefiles"
		fi
	fi
	read -p "done."
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
					read -p "done."
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
				read -p "done."
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
			read -p "done."
			exit 1
		fi
	fi
fi

cmake --build ${vf} --config ${vm} --target ${vp}
if [ $vp == ${vn}_All ]
then
	cmake --build ${vf} --config ${vm} --target ${vn}_CLAP
	if [[ "$OSTYPE" =~ ^msys ]]
	then
		cp "./${vf}/${vn}_artefacts/${vm}/CLAP/${vs}.clap" "../../Setup/${vf}/paid/${vs}.clap"
		cp "./${vf}/${vn}_artefacts/${vm}/Standalone/${vs}.exe" "../../Setup/${vf}/paid/${vs}.exe"
	else
		cp "./${vf}/${vn}_artefacts/CLAP/${vs}.clap" "../../Setup/${vf}/paid/${vs}.clap"
		cp "./${vf}/${vn}_artefacts/Standalone/${vs}" "../../Setup/${vf}/paid/${vs}"
	fi
else
	if [ $vp == ${vn}_CLAP ]
	then
		if [[ "$OSTYPE" =~ ^msys ]]
		then
			cp "./${vf}/${vn}_artefacts/${vm}/CLAP/${vs}.clap" "../../Setup/${vf}/paid/${vs}.clap"
		else
			cp "./${vf}/${vn}_artefacts/CLAP/${vs}.clap" "../../Setup/${vf}/paid/${vs}.clap"
		fi
	else
		if [ $vp == ${vn}_Standalone ]
		then
			cp "./${vf}/${vn}_artefacts/${vm}/Standalone/${vs}.exe" "../../Setup/${vf}/free/${vs}.exe"
		else
			cp "./${vf}/${vn}_artefacts/Standalone/${vs}" "../../Setup/${vf}/free/${vs}"
		fi
	fi
fi
if [ $vr == y ]
then
	if [[ "$OSTYPE" =~ ^msys ]]
	then
		"./${vf}/${vn}_artefacts/${vm}/Standalone/${vs}.exe"
	else
		"./${vf}/${vn}_artefacts/Standalone/${vs}"
	fi
fi

read -p "done."
exit 1
