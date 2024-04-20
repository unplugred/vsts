#!/bin/bash
case $OSTYPE in
	msys)
		vf=build_windows
		;;
	cygwin)
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
			cygwin)
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
		vl=plasticfuneral
		vn=PlasticFuneral
		vs="Plastic Funeral"
		;;
	vu)
		vl=vu
		vn=VU
		vs=VU
		vc=y
		;;
	cl)
		vl=clickbox
		vn=ClickBox
		vs=ClickBox
		va=free
		;;
	pi)
		vl=pisstortion
		vn=Pisstortion
		vs=Pisstortion
		;;
	pn)
		vl=pnch
		vn=PNCH
		vs=PNCH
		;;
	po)
		vl=proto
		vn=Proto
		vs=Proto
		;;
	re)
		vl=redbass
		vn=RedBass
		vs="Red Bass"
		;;
	mp)
		vl=mpaint
		vn=MPaint
		vs=MPaint
		vc=y
		va=free
		;;
	cr)
		vl=crmbl
		vn=CRMBL
		vs=CRMBL
		;;
	pr)
		vl=prisma
		vn=Prisma
		vs=Prisma
		;;
	su)
		vl=sunburnt
		vn=SunBurnt
		vs=SunBurnt
		;;
	ev)
		cmake --build ${vf} --config ${vm} --target PlasticFuneral_All
		cmake --build ${vf} --config ${vm} --target PlasticFuneral_CLAP
		cmake --build ${vf} --config ${vm} --target VU_All
		cmake --build ${vf} --config ${vm} --target VU_CLAP
		cmake --build ${vf} --config ${vm} --target ClickBox_All
		cmake --build ${vf} --config ${vm} --target ClickBox_CLAP
		cmake --build ${vf} --config ${vm} --target Pisstortion_All
		cmake --build ${vf} --config ${vm} --target Pisstortion_CLAP
		cmake --build ${vf} --config ${vm} --target PNCH_All
		cmake --build ${vf} --config ${vm} --target PNCH_CLAP
		cmake --build ${vf} --config ${vm} --target Proto_All
		cmake --build ${vf} --config ${vm} --target Proto_CLAP
		cmake --build ${vf} --config ${vm} --target RedBass_All
		cmake --build ${vf} --config ${vm} --target RedBass_CLAP
		cmake --build ${vf} --config ${vm} --target MPaint_All
		cmake --build ${vf} --config ${vm} --target MPaint_CLAP
		cmake --build ${vf} --config ${vm} --target CRMBL_All
		cmake --build ${vf} --config ${vm} --target CRMBL_CLAP
		cmake --build ${vf} --config ${vm} --target Prisma_All
		cmake --build ${vf} --config ${vm} --target Prisma_CLAP
		cmake --build ${vf} --config ${vm} --target SunBurnt_All
		cmake --build ${vf} --config ${vm} --target SunBurnt_CLAP
		if [[ "$OSTYPE" =~ ^msys ]] || [[ "$OSTYPE" =~ ^cygwin ]]
		then
			cp "./${vf}/plugins/plasticfuneral/PlasticFuneral_artefacts/${vm}/CLAP/Plastic Funeral.clap" "./setup/${vf}/paid/Plastic Funeral.clap"
			cp "./${vf}/plugins/vu/VU_artefacts/${vm}/CLAP/VU.clap" "./setup/${vf}/paid/VU.clap"
			cp "./${vf}/plugins/vu/VU_artefacts/${vm}/Standalone/VU.exe" "./setup/${vf}/paid/VU.exe"
			cp "./${vf}/plugins/clickbox/ClickBox_artefacts/${vm}/CLAP/ClickBox.clap" "./setup/${vf}/free/ClickBox.clap"
			cp "./${vf}/plugins/pisstortion/Pisstortion_artefacts/${vm}/CLAP/Pisstortion.clap" "./setup/${vf}/paid/Pisstortion.clap"
			cp "./${vf}/plugins/pnch/PNCH_artefacts/${vm}/CLAP/PNCH.clap" "./setup/${vf}/paid/PNCH.clap"
			cp "./${vf}/plugins/redbass/RedBass_artefacts/${vm}/CLAP/Red Bass.clap" "./setup/${vf}/paid/Red Bass.clap"
			cp "./${vf}/plugins/mpaint/MPaint_artefacts/${vm}/CLAP/MPaint.clap" "./setup/${vf}/free/MPaint.clap"
			cp "./${vf}/plugins/mpaint/MPaint_artefacts/${vm}/Standalone/MPaint.exe" "./setup/${vf}/free/MPaint.exe"
			cp "./${vf}/plugins/crmbl/CRMBL_artefacts/${vm}/CLAP/CRMBL.clap" "./setup/${vf}/paid/CRMBL.clap"
			cp "./${vf}/plugins/prisma/Prisma_artefacts/${vm}/CLAP/Prisma.clap" "./setup/${vf}/paid/Prisma.clap"
			cp "./${vf}/plugins/sunburnt/SunBurnt_artefacts/${vm}/CLAP/SunBurnt.clap" "./setup/${vf}/paid/SunBurnt.clap"
		else
			cp "./${vf}/plugins/plasticfuneral/PlasticFuneral_artefacts/CLAP/Plastic Funeral.clap" "./setup/${vf}/paid/Plastic Funeral.clap"
			cp "./${vf}/plugins/vu/VU_artefacts/CLAP/VU.clap" "./setup/${vf}/paid/VU.clap"
			cp "./${vf}/plugins/vu/VU_artefacts/Standalone/VU" "./setup/${vf}/paid/VU"
			cp "./${vf}/plugins/clickbox/ClickBox_artefacts/CLAP/ClickBox.clap" "./setup/${vf}/free/ClickBox.clap"
			cp "./${vf}/plugins/pisstortion/Pisstortion_artefacts/CLAP/Pisstortion.clap" "./setup/${vf}/paid/Pisstortion.clap"
			cp "./${vf}/plugins/pnch/PNCH_artefacts/CLAP/PNCH.clap" "./setup/${vf}/paid/PNCH.clap"
			cp "./${vf}/plugins/redbass/RedBass_artefacts/CLAP/Red Bass.clap" "./setup/${vf}/paid/Red Bass.clap"
			cp "./${vf}/plugins/mpaint/MPaint_artefacts/CLAP/MPaint.clap" "./setup/${vf}/free/MPaint.clap"
			cp "./${vf}/plugins/mpaint/MPaint_artefacts/Standalone/MPaint" "./setup/${vf}/free/MPaint"
			cp "./${vf}/plugins/crmbl/CRMBL_artefacts/CLAP/CRMBL.clap" "./setup/${vf}/paid/CRMBL.clap"
			cp "./${vf}/plugins/prisma/Prisma_artefacts/CLAP/Prisma.clap" "./setup/${vf}/paid/Prisma.clap"
			cp "./${vf}/plugins/sunburnt/SunBurnt_artefacts/CLAP/SunBurnt.clap" "./setup/${vf}/paid/SunBurnt.clap"
		fi
		read -p "done."
		exit 1
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
	if [[ "$OSTYPE" =~ ^msys ]] || [[ "$OSTYPE" =~ ^cygwin ]]
	then
		cp "./${vf}/plugins/${vl}/${vn}_artefacts/${vm}/CLAP/${vs}.clap" "./setup/${vf}/${va}/${vs}.clap"
		if [ $vc == y ]
		then
			cp "./${vf}/plugins/${vl}/${vn}_artefacts/${vm}/Standalone/${vs}.exe" "./setup/${vf}/${va}/${vs}.exe"
		fi
	else
		cp "./${vf}/plugins/${vl}/${vn}_artefacts/CLAP/${vs}.clap" "./setup/${vf}/${va}/${vs}.clap"
		if [ $vc == y ]
		then
			cp "./${vf}/plugins/${vl}/${vn}_artefacts/Standalone/${vs}" "./setup/${vf}/${va}/${vs}"
		fi
	fi
else
	if [ $vp == ${vn}_CLAP ]
	then
		if [[ "$OSTYPE" =~ ^msys ]] || [[ "$OSTYPE" =~ ^cygwin ]]
		then
			cp "./${vf}/plugins/${vl}/${vn}_artefacts/${vm}/CLAP/${vs}.clap" "./setup/${vf}/${va}/${vs}.clap"
		else
			cp "./${vf}/plugins/${vl}/${vn}_artefacts/CLAP/${vs}.clap" "./setup/${vf}/${va}/${vs}.clap"
		fi
	else
		if [ $vp == ${vn}_Standalone ]
		then
			if [ $vc == y ]
			then
				if [[ "$OSTYPE" =~ ^msys ]] || [[ "$OSTYPE" =~ ^cygwin ]]
				then
					cp "./${vf}/plugins/${vl}/${vn}_artefacts/${vm}/Standalone/${vs}.exe" "./setup/${vf}/${va}/${vs}.exe"
				else
					cp "./${vf}/plugins/${vl}/${vn}_artefacts/Standalone/${vs}" "./setup/${vf}/${va}/${vs}"
				fi
			fi
		fi
	fi
fi
if [ $vr == y ]
then
	if [[ "$OSTYPE" =~ ^msys ]] || [[ "$OSTYPE" =~ ^cygwin ]]
	then
		"./${vf}/plugins/${vl}/${vn}_artefacts/${vm}/Standalone/${vs}.exe"
	else
		"./${vf}/plugins/${vl}/${vn}_artefacts/Standalone/${vs}"
	fi
fi
