#!/bin/bash
case $OSTYPE in
	msys)
		vf=build_windows
		;;
	darwin)
		vf=build_mac
		;;
	*)
		vf=build_linux
		;;
esac

vm=null
read -p "[c]onfigure/[D]ebug/[r]elease/[m]in size rel/rel [w]ith deb info: " -n 1 -r
REPLY=${REPLY:-d}
echo
case $REPLY in
	c)
		vb=null
		read -p "[f]ree/[P]aid/[b]eta: " -n 1 -r
		REPLY=${REPLY:-p}
		echo
		case $REPLY in
			p)
				vb=0
				;;
			f)
				vb=1
				;;
			b)
				vb=2
				;;
			*)
				read -p "done."
				exit 1
				;;
		esac

		case $OSTYPE in
			msys)
				cmake -DBANNERTYPE=${vb} -B ${vf} -G "Visual Studio 17 2022" -T host=x64 -A x64
				;;
			darwin)
				cmake -DBANNERTYPE=${vb} -B ${vf} -G "Xcode"
				;;
			*)
				cmake -DBANNERTYPE=${vb} -B ${vf} -G "Unix Makefiles"
				;;
		esac
		read -p "done."
		exit 1
		;;
	d)
		vm=Debug
		;;
	r)
		vm=Release
		;;
	m)
		vm=MinSizeRel
		;;
	w)
		vm=RelWithDebInfo
		;;
	*)
		read -p "done."
		exit 1
		;;
esac

va=paid
vc=n
read -p "Plugin code: " -n 2 -r
echo
case $REPLY in
	pl)
		vn=PlasticFuneral
		vs="Plastic Funeral"
		;;
	vu)
		vn=VU
		vs=VU
		vc=y
		;;
	cl)
		vn=ClickBox
		vs=ClickBox
		va=free
		;;
	pi)
		vn=Pisstortion
		vs=Pisstortion
		;;
	pn)
		vn=PNCH
		vs=PNCH
		;;
	re)
		vn=RedBass
		vs="Red Bass"
		;;
	mp)
		vn=MPaint
		vs=MPaint
		vc=y
		va=free
		;;
	cr)
		vn=CRMBL
		vs=CRMBL
		;;
	pr)
		vn=Prisma
		vs=Prisma
		;;
	*)
		read -p "done."
		exit 1
		;;
esac

vp=null
vr=n
read -p "[a]ll build/[S]tandalone/[v]st3/[c]lap: " -n 1 -r
REPLY=${REPLY:-s}
echo
case $REPLY in
	a)
		vp=${vn}_All
		vr=m
		;;
	s)
		vp=${vn}_Standalone
		vr=m
		;;
	v)
		vp=${vn}_VST3
		;;
	c)
		vp=${vn}_CLAP
		;;
	*)
		read -p "done."
		exit 1
		;;
esac

if [ $vr == m ]
then
	read -p "run after compiling? ([Y]es/[n]o): " -n 1 -r
	REPLY=${REPLY:-y}
	echo
	case $REPLY in
		y)
			vr=y
			;;
		n)
			vr=n
			;;
		*)
			read -p "done."
			exit 1
			;;
	esac
fi

cmake --build ${vf} --config ${vm} --target ${vp}
if [ $vp == ${vn}_All ]
then
	cmake --build ${vf} --config ${vm} --target ${vn}_CLAP
	if [[ "$OSTYPE" =~ ^msys ]]
	then
		cp "./${vf}/Plugins/${vs}/${vn}_artefacts/${vm}/CLAP/${vs}.clap" "./Setup/${vf}/${va}/${vs}.clap"
		if [ $vc == y ]
		then
			cp "./${vf}/Plugins/${vs}/${vn}_artefacts/${vm}/Standalone/${vs}.exe" "./Setup/${vf}/${va}/${vs}.exe"
		fi
	else
		cp "./${vf}/Plugins/${vs}/${vn}_artefacts/CLAP/${vs}.clap" "./Setup/${vf}/${va}/${vs}.clap"
		if [ $vc == y ]
		then
			cp "./${vf}/Plugins/${vs}/${vn}_artefacts/Standalone/${vs}" "./Setup/${vf}/${va}/${vs}"
		fi
	fi
else
	if [ $vp == ${vn}_CLAP ]
	then
		if [[ "$OSTYPE" =~ ^msys ]]
		then
			cp "./${vf}/Plugins/${vs}/${vn}_artefacts/${vm}/CLAP/${vs}.clap" "./Setup/${vf}/${va}/${vs}.clap"
		else
			cp "./${vf}/Plugins/${vs}/${vn}_artefacts/CLAP/${vs}.clap" "./Setup/${vf}/${va}/${vs}.clap"
		fi
	else
		if [ $vp == ${vn}_Standalone ]
		then
			if [ $vc == y ]
			then
				if [[ "$OSTYPE" =~ ^msys ]]
				then
					cp "./${vf}/Plugins/${vs}/${vn}_artefacts/${vm}/Standalone/${vs}.exe" "./Setup/${vf}/${va}/${vs}.exe"
				else
					cp "./${vf}/Plugins/${vs}/${vn}_artefacts/Standalone/${vs}" "./Setup/${vf}/${va}/${vs}"
				fi
			fi
		fi
	fi
fi
if [ $vr == y ]
then
	if [[ "$OSTYPE" =~ ^msys ]]
	then
		"./${vf}/Plugins/${vs}/${vn}_artefacts/${vm}/Standalone/${vs}.exe"
	else
		"./${vf}/Plugins/${vs}/${vn}_artefacts/Standalone/${vs}"
	fi
fi
