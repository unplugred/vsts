import os
import sys
import shutil
import json
import shlex
from zipfile import ZipFile

os.chdir(os.path.dirname(os.path.abspath(__file__)))

codes = {
	"version": ["free","paid","beta"],
	"plugin": [],
	"config": ["Debug","Release"],
	"target": [],
	"run": ["yes","no"],
	"system": []
}

systems = [{
	"name": "Mac",
	"code": "mac",
	"image": "macos-13",
	"compiler": "Xcode",
	"executable": ".app"
},{
	"name": "Win64",
	"code": "win",
	"image": "windows-latest",
	"compiler": "Visual Studio 17 2022",
	"executable": ".exe"
},{
	"name": "Linux",
	"code": "linux",
	"image": "ubuntu-22.04",
	"compiler": "Unix Makefiles",
	"executable": ""
}]
for i in range(len(systems)):
	codes["system"].append(systems[i]["name"])
if sys.platform == "darwin":
	system = 0
	import pty
elif sys.platform == "win32" or sys.platform == "cygwin":
	system = 1
	import subprocess
else:
	system = 2
	import pty

plugins = [{
	"name": "Plastic Funeral",
	"code": "Vctz"
},{
	"name": "VU",
	"code": "V6sd",
	"standalone": True
},{
	"name": "ClickBox",
	"code": "Poc9",
	"paid": False
},{
	"name": "Pisstortion",
	"code": "Piss"
},{
	"name": "PNCH",
	"code": "Pnch"
},{
	"name": "Proto",
	"code": "Prtt",
	"paid": False,
	"in_bundle": False
},{
	"name": "Red Bass",
	"code": "Rdbs"
},{
	"name": "MPaint",
	"code": "Mpnt",
	"standalone": True,
	"paid": False,
	"au_type": "aumu",
	"additional_files": [{
		"path": "samples",
		"output": "MPaint samples",
		"versions": ["mac_free","win_free","linux_free"],
		"copy": True
	},{
		"path": "how_to_install_mpaint_mac.txt",
		"output": "Manual install/how to install mpaint.txt",
		"versions": ["mac_free"],
		"copy": False
	},{
		"path": "how_to_install_mpaint.txt",
		"output": "Manual install/how to install mpaint.txt",
		"versions": ["win_free"],
		"copy": False
	},{
		"path": "how_to_install_mpaint.txt",
		"output": "how to install mpaint.txt",
		"versions": ["linux_free"],
		"copy": False
	}]
},{
	"name": "CRMBL",
	"code": "Crmb"
},{
	"name": "Prisma",
	"code": "Prsm",
	"bundle": "Prisma"
},{
	"name": "Prismon",
	"code": "Prmn",
	"bundle": "Prisma"
},{
	"name": "SunBurnt",
	"code": "Snbt"
},{
	"name": "Diet Audio",
	"code": "Diet"
},{
	"name": "Scope",
	"code": "Scop",
	"standalone": True
},{
	"name": "Magic Carpet",
	"code": "Mgct"
},{
	"name": "ModMan",
	"code": "Mdmn"
}]
for i in range(len(plugins)):
	if "standalone" not in plugins[i]:
		plugins[i]["standalone"] = False
	if "paid" not in plugins[i]:
		plugins[i]["paid"] = True
	if "au_type" not in plugins[i]:
		plugins[i]["au_type"] = "aufx"
	if "additional_files" not in plugins[i]:
		plugins[i]["additional_files"] = []
	if "in_bundle" not in plugins[i]:
		plugins[i]["in_bundle"] = True
	codes["plugin"].append(plugins[i]["name"])

targets = [{
	"name": "Audio Unit",
	"code": "AU",
	"extension": ".component",
	"description": "For Logic and GarageBand",
	"mac_location": "/Library/Audio/Plug-Ins/Components"
},{
	"name": "CLAP",
	"code": "CLAP",
	"extension": ".clap",
	"description": "For Reaper and Bitwig (currently)",
	"mac_location": "/Library/Audio/Plug-Ins/CLAP/UnplugRed"
},{
	"name": "VST3",
	"code": "VST3",
	"extension": ".vst3",
	"description": "For everything else",
	"mac_location": "/Library/Audio/Plug-Ins/VST3/UnplugRed"
},{
	"name": "Standalone",
	"code": "Standalone",
	"extension": systems[system]["executable"],
	"description": "For you. No DAW required",
	"mac_location": "/Applications"
}]
for i in range(len(targets)):
	codes["target"].append(targets[i]["name"])

valid_secrets = [
	"KEYCHAIN_PASSWORD",
	"DEV_ID_APP_CERT",
	"DEV_ID_APP_PASSWORD",
	"DEV_ID_INSTALL_CERT",
	"DEV_ID_INSTALL_PASSWORD",
	"NOTARIZATION_USERNAME",
	"NOTARIZATION_PASSWORD",
	"TEAM_ID",
	"DEVELOPER_ID_APPLICATION",
	"DEVELOPER_ID_INSTALLER",
]
saved_data = { "is_free": [], "secrets": {}, "codesign_plugins": True }
for i in range(len(systems)):
	saved_data["is_free"].append(False)
if os.path.isfile("saved_data.json"):
	with open("saved_data.json",'r') as open_file:
		saved_data = json.load(open_file)


def fuzzy_match(term,data):
	lower = str(term).replace(' ','').lower()
	if lower != "":
		for result in data:
			if result.replace(' ','').lower().startswith(lower):
				return result

	return None

def get_plugin(term):
	lower = str(term).replace(' ','').lower()
	if lower != "":
		for plugin in plugins:
			if plugin["name"].replace(' ','').lower().startswith(lower):
				return plugin

	error("Unknown plugin: "+term)

def get_system(term):
	lower = str(term).replace(' ','').lower()
	if lower != "":
		for i in range(len(systems)):
			if systems[i]["name"].replace(' ','').lower().startswith(lower) or str(i) == lower:
				return systems[i]

	error("Unknown system: "+term)

def get_target(term):
	lower = str(term).replace(' ','').lower()
	if lower != "":
		for target in targets:
			if target["name"].replace(' ','').lower().startswith(lower):
				return target

	error("Unknown target: "+term)


def debug(string):
	print('\033[1m'+string+'\033[0m')

def alert(string):
	print('\033[1m\033[93m'+string+'\033[0m')

def error(string, exit_code=1):
	print('\033[1m\033[91m'+string+'\033[0m')
	#sys.exit(exit_code)
	sys.exit(1)

def run_command(cmd,ignore_errors=False):
	censored_command = cmd
	for secret in saved_data["secrets"].values():
		censored_command = censored_command.replace(secret,"***")
	debug("RUNNING COMMAND: "+censored_command)

	if systems[system]["code"] == "win":
		os.environ['SYSTEMD_COLORS'] = '1'
		process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
		while process.poll() == None:
			print(process.stdout.readline().decode("UTF-8"),end='')
		return_code = process.returncode
	else:
		return_code = pty.spawn(shlex.split(cmd), lambda fd: os.read(fd, 1024))

	if return_code != 0:
		if ignore_errors:
			alert("Exited with return code "+str(return_code))
		else:
			error("Exited with return code "+str(return_code)+", exiting...",return_code)

def join(arr):
	return '/'.join(arr)

def move(path, output):
	debug("MOVING PATH "+path+" TO "+output)

	if not os.path.exists(path):
		error("Invalid path: "+path)
	if not os.path.isdir(os.path.dirname(output)):
		error("Invalid path: "+output)

	os.replace(path, output)

def copy(path, output):
	debug("COPYING PATH "+path+" TO "+output)

	if not os.path.exists(path):
		error("Invalid path: "+path)
	if output.endswith('/'):
		if not os.path.isdir(os.path.abspath(output+"..")):
			error("Invalid path: "+output)
	elif not os.path.isdir(os.path.dirname(output)):
		error("Invalid path: "+output)

	if os.path.isdir(path):
		if os.path.exists(output):
			shutil.rmtree(output)
		shutil.copytree(path,output)
	else:
		shutil.copy2(path, output)

def remove(path):
	debug("REMOVING PATH "+path)

	if not os.path.exists(path):
		alert("Path already removed: "+path)
		return

	if os.path.isdir(path):
		shutil.rmtree(path)
	else:
		os.remove(path)

def create_dir(path):
	debug("CREATING DIRECTORY "+path)

	if os.path.isdir(path):
		alert("Directory "+path+" already exists.")
		return

	os.makedirs(path)

def zip_files(path, output, zip_level=6):
	debug("ZIPPING "+path+" TO "+output)

	if not os.path.exists(path):
		error("Invalid path: "+path)
	if not os.path.isdir(os.path.dirname(output)) or not output.endswith(".zip"):
		error("Invalid path: "+output)

	with ZipFile(output,'w',compresslevel=zip_level) as zf:
		if os.path.isfile(path):
			zf.write(path)
		else:
			for root, dirs, files in os.walk(path):
				for file in files:
					if path.endswith('/') or path.endswith('\\'):
						zf.write(join([root,file]),os.path.relpath(join([root,file]),path))
					else:
						zf.write(join([root,file]),os.path.relpath(join([root,file]),os.path.dirname(path)))

def unzip_files(path, output=None):
	if output != None:
		debug("UNZIPPING "+path+" TO "+output)
	else:
		debug("UNZIPPING "+path)
		output = os.path.dirname(path)

	if not os.path.isfile(path) or not path.endswith(".zip"):
		error("Invalid path: "+path)
	if not os.path.isdir(os.path.dirname(output)):
		error("Invalid path: "+output)

	with ZipFile(path,'r') as zf:
		zf.extractall(path=output)

def ls(path):
	debug("LISTING FILES IN "+path)
	for file in os.listdir(path):
		debug(file)

def product_build(plugin):
	debug("GENERATING PRODUCTBUILD XML")
	nospace = plugin.replace(' ','')
	lower = nospace.lower()
	version_tag = ""
	if plugin == "Everything Bundle":
		standalone = True
	else:
		standalone = False
		if "bundle" in get_plugin(plugin):
			for bundle_plugin in plugins:
				if "bundle" in bundle_plugin and bundle_plugin["bundle"] == get_plugin(plugin)["bundle"] and bundle_plugin["standalone"]:
					standalone = True
		else:
			standalone = get_plugin(plugin)["standalone"]
		if get_plugin(plugin)["paid"] and saved_data["is_free"][system]:
			version_tag = "-free"

	file = open(lower+version_tag+".xml","w")
	file.write('''<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
	<title>'''+plugin+''' Install</title>
	<domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
	<allowed-os-versions>
		<os-version min="10.13"/>
	</allowed-os-versions>
	<options customize="always" require-scripts="false" rootVolumeOnly="true"/>
	<welcome file="pages/welcome.html" mime-type="text/html"/>
	<conclusion file="pages/conclusion.html" mime-type="text/html"/>
	<choices-outline>''')
	for target in targets:
		if target["name"] == "Standalone" and not standalone:
			continue
		file.write('''
		<line choice="'''+target["code"].lower()+'''"/>''')
	file.write('''
	</choices-outline>''')
	for target in targets:
		if target["name"] == "Standalone" and not standalone:
			continue
		file.write('''
	<choice id="'''+target["code"].lower()+"\" title=\""+target["name"]+"\" description=\""+target["description"]+'''">
		<pkg-ref id="com.unplugred.'''+nospace+version_tag+"-"+target["code"].lower()+"\">"+nospace+version_tag+"-"+target["code"].lower()+'''.pkg</pkg-ref>
	</choice>''')
	file.write('''
</installer-gui-script>
''')
	file.close()

	if standalone:
		debug("GENERATING COMPONENT.PLIST")
		file = open("component.plist","w")
		file.write('''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
	<array>''')
		for aplugin in (plugins if plugin == "Everything Bundle" else [get_plugin(plugin)]):
			if aplugin["in_bundle"] and aplugin["standalone"]:
				file.write('''
		<dict>
			<key>BundleHasStrictIdentifier</key>
			<true/>
			<key>BundleIsRelocatable</key>
			<false/>
			<key>BundleIsVersionChecked</key>
			<false/>
			<key>BundleOverwriteAction</key>
			<string>upgrade</string>
			<key>RootRelativeBundlePath</key>
			<string>'''+aplugin["name"]+'''.app</string>
		</dict>''')
		file.write('''
	</array>
</plist>
''')
		file.close()

def prepare():
	debug("PREPARING DEPENDENCIES")
	if systems[system]["code"] == "mac":
		run_command("sudo xcode-select -s '/Applications/Xcode_15.1.app/Contents/Developer'")
	elif systems[system]["code"] == "linux":
		run_command("sudo apt-get update",True)
		run_command("sudo apt-get install -y g++-11 gcc-11 libasound2-dev libjack-jackd2-dev ladspa-sdk libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev",True)
		run_command("sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100 --slave /usr/bin/g++ g++ /usr/bin/g++-11 --slave /usr/bin/gcov gcov /usr/bin/gcov-11",True)

def update_secrets(secrets):
	debug("UPDATING SECRETS")
	for secret in secrets:
		split = secret.split('=',1)
		if split[0].upper() in valid_secrets:
			saved_data["secrets"][split[0].upper()] = split[1]
		else:
			error("Unknown secret: "+split[0])
	with open("saved_data.json","w") as out_file:
		json.dump(saved_data,out_file);

def configure(version):
	debug("CONFIGURING "+version.upper()+" VERSION")
	saved_data["is_free"][system] = version != "paid"
	with open("saved_data.json","w") as out_file:
		json.dump(saved_data,out_file);

	version_num = 2
	if version == "free":
		version_num = 0
	elif version == "beta":
		version_num = 1
	cmd = "cmake -DBANNERTYPE="+str(version_num)+" -B \"build_"+systems[system]["code"]+"\" -G "
	if systems[system]["code"] == "win":
		run_command(cmd+"\""+systems[system]["compiler"]+"\" -T host=x64 -A x64")
	else:
		run_command(cmd+"\""+systems[system]["compiler"]+"\"")

def build(plugin, config, target):
	debug("BUILDING "+plugin.upper()+", CONFIG "+config.upper()+", TARGET "+target.upper())

	if target == "Audio Unit" and systems[system]["code"] != "mac":
		alert("Cannot build audio unit on "+systems[system]["name"]+", skipping...")
		return

	nospace = plugin.replace(' ','')
	lower = nospace.lower()
	free = "free" if (saved_data["is_free"][system] or not get_plugin(plugin)["paid"]) else "paid"
	folder = "build_"+systems[system]["code"]
	file_extension = get_target(target)["extension"]

	create_dir(join(["setup",folder,free]))
	target_path = join(["setup",folder,free,plugin+file_extension])

	if systems[system]["code"] == "mac":
		create_dir(join([folder,"plugins",("" if "bundle" not in get_plugin(plugin) else (get_plugin(plugin)["bundle"].replace(' ','').lower()+"/"))+lower,nospace+"_artefacts",config,target]))
		run_command("cmake --build \""+folder+"\" --config "+config+" --target "+nospace+"_"+get_target(target)["code"])
		if target == "CLAP" or target == "Standalone":
			copy(join([folder,"plugins",("" if "bundle" not in get_plugin(plugin) else (get_plugin(plugin)["bundle"].replace(' ','').lower()+"/"))+lower,nospace+"_artefacts",config,target,plugin+file_extension]),target_path)
			if target == "Standalone":
				run_command("chmod -R 755 \""+join([target_path,"Contents","MacOS",plugin])+"\"")
				run_command("chmod -R 755 \""+target_path+"\"")
		if saved_data["secrets"] != {} and saved_data["codesign_plugins"]:
			zip_path = join(["setup",folder,free,lower+("_free_" if saved_data["is_free"][system] and get_plugin(plugin)["paid"] else "_")+get_target(target)["code"].lower()+".zip"])
			run_command("codesign --force -s \""+saved_data["secrets"]["DEVELOPER_ID_APPLICATION"]+"\" -v \""+target_path+"\" --deep --strict --options=runtime --timestamp")
			zip_files(target_path,zip_path)
			remove(target_path)
			run_command("xcrun notarytool submit \""+zip_path+"\" --apple-id "+saved_data["secrets"]["NOTARIZATION_USERNAME"]+" --password "+saved_data["secrets"]["NOTARIZATION_PASSWORD"]+" --team-id "+saved_data["secrets"]["TEAM_ID"]+" --wait")
			unzip_files(zip_path)
			remove(zip_path)
			run_command("xcrun stapler staple \""+target_path+"\"")
			if target == "Audio Unit":
				run_command("sudo cp -R -f \""+target_path+"\" \""+join([get_target(target)["mac_location"],plugin+file_extension+"\""]))
				run_command("killall -9 AudioComponentRegistrar",True)
				run_command("auval -a")
				run_command("auval -strict -v "+get_plugin(plugin)["au_type"]+" "+get_plugin(plugin)["code"]+" Ured")
			elif target == "Standalone":
				run_command("chmod -R 755 \""+join([target_path,"Contents","MacOS",plugin])+"\"")
				run_command("chmod -R 755 \""+target_path+"\"")

	elif systems[system]["code"] == "win":
		run_command("cmake --build \""+folder+"\" --config "+config+" --target "+nospace+"_"+get_target(target)["code"])
		if target == "VST3":
			move(join([target_path,"Contents","x86_64-win",plugin+file_extension]),join(["setup",plugin+file_extension]))
			remove(target_path)
			move(join(["setup",plugin+file_extension]),target_path)
		elif target == "CLAP" or target == "Standalone":
			copy(join([folder,"plugins",("" if "bundle" not in get_plugin(plugin) else (get_plugin(plugin)["bundle"].replace(' ','').lower()+"/"))+lower,nospace+"_artefacts",config,target,plugin+file_extension]),target_path)
		for file in get_plugin(plugin)["additional_files"]:
			if ("win_"+free) in file["versions"] and file["copy"] and systems[system]["code"] == "win":
				if not os.path.isdir(join(["setup",folder,"other",plugin])):
					create_dir(join(["setup",folder,"other",plugin]))
				copy(join(["plugins",lower,file["path"]]),join(["setup",folder,"other",plugin,file["output"]]))

	else:
		run_command("cmake --build \""+folder+"\" --config "+config+" --target "+nospace+"_"+get_target(target)["code"])
		if target == "CLAP" or target == "Standalone":
			copy(join([folder,"plugins",("" if "bundle" not in get_plugin(plugin) else (get_plugin(plugin)["bundle"].replace(' ','').lower()+"/"))+lower,nospace+"_artefacts",target,plugin+file_extension]),target_path)

def run_plugin(plugin, config):
	artefact_path = join(["build_"+systems[system]["code"],"plugins",("" if "bundle" not in get_plugin(plugin) else (get_plugin(plugin)["bundle"].replace(' ','').lower()+"/"))+plugin.replace(' ','').lower(),plugin.replace(' ','')+"_artefacts"])
	if systems[system]["code"] == "mac":
		run_command("open -W \""+join([artefact_path,config,"Standalone",plugin+systems[system]["executable"]])+"\"")
	elif systems[system]["code"] == "win":
		run_command("\""+join([artefact_path,config,"Standalone",plugin+systems[system]["executable"]])+"\"")
	else:
		run_command("\""+join([artefact_path,"Standalone",plugin+systems[system]["executable"]])+"\"")

def build_installer(plugin, system_i, zip_result=True):
	debug("BUILDING INSTALLER FOR "+plugin.upper())

	nospace = plugin.replace(' ','')
	lower = nospace.lower()
	free = "free" if (saved_data["is_free"][system] or not get_plugin(plugin)["paid"]) else "paid"
	folder = "build_"+get_system(system_i)["code"]

	remove(join(["setup","temp"]))
	create_dir(join(["setup","temp"]))
	if get_system(system_i)["code"] == "mac" or get_system(system_i)["code"] == "win":
		create_dir(join(["setup","temp","Manual install"]))

	bundle_out = []
	if "bundle" in get_plugin(plugin):
		for bundle_plugin in plugins:
			if bundle_plugin["name"] == plugin or "bundle" not in bundle_plugin:
				continue
			if bundle_plugin["bundle"] == get_plugin(plugin)["bundle"]:
				bundle_out.append(bundle_plugin["name"])

	def if_free(str):
		if get_plugin(plugin)["paid"] and saved_data["is_free"][system]:
			return str
		else:
			return ""
	installer = plugin+if_free(" Free")+" Installer"

	other_data = False
	for file in get_plugin(plugin)["additional_files"]:
		if (get_system(system_i)["code"]+"_"+free) in file["versions"] and file["copy"] and get_system(system_i)["code"] != "linux":
			copy(join(["plugins",lower,file["path"]]),join(["setup","temp","Manual install",file["output"]]))
			if get_system(system_i)["code"] == "win":
				other_data = True

	if get_system(system_i)["code"] == "mac":
		identifier = nospace+if_free("-free")
		standalone = False
		product_build(plugin)
		for target in targets:
			for bundle_plugin in (bundle_out+[plugin]):
				if target["name"] == "Standalone":
					if not get_plugin(bundle_plugin)["standalone"]:
						continue
					else:
						standalone = True
				copy(join(["setup",folder,free,bundle_plugin+target["extension"]]),join(["setup","temp","Manual install",bundle_plugin+target["extension"]]))
			if target["name"] == "Standalone" and not standalone:
				continue
			if target["name"] == "Audio Unit":
				run_command("chmod +rx \""+join(["setup","assets","scripts","postinstall"])+"\"")
			run_command("pkgbuild --install-location \""+target["mac_location"]+"\" --identifier \"com.unplugred."+identifier+"-"+target["code"].lower()+".pkg\""+((" --scripts \""+join(["setup","assets","scripts"])+"\"") if target["name"] == "Audio Unit" else "")+" --version 1.1.1 --root \""+join(["setup","temp","Manual install"])+"\""+(" --component-plist \"component.plist\"" if target["name"] == "Standalone" else "")+" \""+identifier+"-"+target["code"].lower()+".pkg\"")
			for bundle_plugin in (bundle_out+[plugin]):
				if target["name"] == "Standalone" and not get_plugin(bundle_plugin)["standalone"]:
					continue
				move(join(["setup","temp","Manual install",bundle_plugin+target["extension"]]),join(["setup","temp",bundle_plugin+target["extension"]]))
		run_command("productbuild --timestamp --sign \""+saved_data["secrets"]["DEVELOPER_ID_INSTALLER"]+"\" --distribution \""+identifier+".xml\" --resources \""+join(["setup","assets",""])+"\" \""+installer+".pkg\"")
		run_command("xcrun notarytool submit \""+installer+".pkg\" --apple-id "+saved_data["secrets"]["NOTARIZATION_USERNAME"]+" --password "+saved_data["secrets"]["NOTARIZATION_PASSWORD"]+" --team-id "+saved_data["secrets"]["TEAM_ID"]+" --wait")
		run_command("xcrun stapler staple \""+installer+".pkg\"")
		for target in targets:
			for bundle_plugin in (bundle_out+[plugin]):
				if get_plugin(bundle_plugin)["standalone"] or target["name"] != "Standalone":
					move(join(["setup","temp",bundle_plugin+target["extension"]]),join(["setup","temp","Manual install",bundle_plugin+target["extension"]]))
		move(installer+".pkg",join(["setup","temp",installer+".pkg"]))
		remove(identifier+".xml")
		for target in targets:
			if not target["name"] == "Standalone" or standalone:
				if target["name"] == "Standalone":
					remove("component.plist")
				remove(identifier+"-"+target["code"].lower()+".pkg")

	elif get_system(system_i)["code"] == "win":
		imagemissing = not os.path.exists(join(["setup","assets","image",plugin+".bmp"]))
		smallimagemissing = not os.path.exists(join(["setup","assets","smallimage",plugin+".bmp"]))
		if imagemissing:
			copy(join(["setup","assets","image","Proto.bmp"]),join(["setup","assets","image",plugin+".bmp"]))
		if smallimagemissing:
			copy(join(["setup","assets","smallimage","Proto.bmp"]),join(["setup","assets","smallimage",plugin+".bmp"]))
		cmd = "iscc \""+join(["setup","innosetup.iss"])+"\" \"/DPluginName="+plugin+"\" \"/DVersion="+free+"\""
		if not get_plugin(plugin)["paid"]:
			cmd += " \"/DNoPaid\""
		if get_plugin(plugin)["standalone"]:
			cmd += " \"/DStandalone\""
		if other_data:
			cmd += " \"/DOtherData\""
		if bundle_out != []:
			cmd += " \"/DBundle="+'_'.join(bundle_out)+"\""
		run_command(cmd)
		if imagemissing:
			remove(join(["setup","assets","image",plugin+".bmp"]))
		if smallimagemissing:
			remove(join(["setup","assets","smallimage",plugin+".bmp"]))
		for target in targets:
			if target["name"] == "Audio Unit":
				continue
			for bundle_plugin in (bundle_out+[plugin]):
				if get_plugin(bundle_plugin)["standalone"] or target["name"] != "Standalone":
					copy(join(["setup",folder,free,bundle_plugin+target["extension"]]),join(["setup","temp","Manual install",bundle_plugin+target["extension"]]))
		move(join(["setup","Output",installer+".exe"]),join(["setup","temp",installer+".exe"]))

	else:
		for target in targets:
			if target["name"] == "Audio Unit":
				continue
			for bundle_plugin in (bundle_out+[plugin]):
				if get_plugin(bundle_plugin)["standalone"] or target["name"] != "Standalone":
					copy(join(["setup",folder,free,bundle_plugin+target["extension"]]),join(["setup","temp",bundle_plugin+target["extension"]]))

	for file in get_plugin(plugin)["additional_files"]:
		if (get_system(system_i)["code"]+"_"+free) in file["versions"] and not (file["copy"] and get_system(system_i)["code"] != "linux"):
			copy(join(["plugins",lower,file["path"]]),join(["setup","temp",file["output"]]))

	if zip_result:
		create_dir(join(["setup","zips"]))
		zip_files(join(["setup","temp",""]),join(["setup","zips",plugin+if_free(" Free")+" "+get_system(system_i)["name"]+".zip"]),9)

def build_everything_bundle(system_i):
	debug("BUILDING EVERYTHING BUNDLE")

	remove(join(["setup","temp"]))
	create_dir(join(["setup","temp"]))
	for plugin in plugins:
		if plugin["in_bundle"] and ("bundle" not in plugin or plugin["bundle"] == plugin["name"]):
			unzip_files(join(["setup","zips",plugin["name"]+" "+get_system(system_i)["name"]+".zip"]),join(["setup","temp"]))

	if get_system(system_i)["code"] == "mac" and systems[system]["code"] == "mac":
		debug("BUILDING INSTALLER FOR EVERYTHING BUNDLE")
		identifier = "everythingbundle"
		product_build("Everything Bundle")
		for plugin in plugins:
			if plugin["in_bundle"] and plugin["standalone"]:
				run_command("chmod -R 755 \""+join(["setup","temp","Manual install",plugin["name"]+get_target("Standalone")["extension"],"Contents","MacOS",plugin["name"]])+"\"")
				run_command("chmod -R 755 \""+join(["setup","temp","Manual install",plugin["name"]+get_target("Standalone")["extension"]])+"\"")
			remove(join(["setup","temp",plugin["name"]+" Installer.pkg"]))
			for file in plugin["additional_files"]:
				if (get_system(system_i)["code"]+"_"+("paid" if plugin["paid"] else "free")) in file["versions"] and not file["copy"]:
					remove(join(["setup","temp",file["output"]]))
			for target in targets:
				if plugin["in_bundle"] and (plugin["standalone"] or target["name"] != "Standalone"):
					move(join(["setup","temp","Manual install",plugin["name"]+target["extension"]]),join(["setup","temp",plugin["name"]+target["extension"]]))
		for target in targets:
			for plugin in plugins:
				if plugin["in_bundle"] and (plugin["standalone"] or target["name"] != "Standalone"):
					move(join(["setup","temp",plugin["name"]+target["extension"]]),join(["setup","temp","Manual install",plugin["name"]+target["extension"]]))
			if target["name"] == "Audio Unit":
				run_command("chmod +rx \""+join(["setup","assets","scripts","postinstall"])+"\"")
			run_command("pkgbuild --install-location \""+target["mac_location"]+"\" --identifier \"com.unplugred."+identifier+"-"+target["code"].lower()+".pkg\""+((" --scripts \""+join(["setup","assets","scripts"])+"\"") if target["name"] == "Audio Unit" else "")+" --version 1.1.1 --root \""+join(["setup","temp","Manual install"])+"\""+(" --component-plist \"component.plist\"" if target["name"] == "Standalone" else "")+" \""+identifier+"-"+target["code"].lower()+".pkg\"")
			for plugin in plugins:
				if plugin["in_bundle"] and (plugin["standalone"] or target["name"] != "Standalone"):
					move(join(["setup","temp","Manual install",plugin["name"]+target["extension"]]),join(["setup","temp",plugin["name"]+target["extension"]]))
		run_command("productbuild --timestamp --sign \""+saved_data["secrets"]["DEVELOPER_ID_INSTALLER"]+"\" --distribution \""+identifier+".xml\" --resources \""+join(["setup","assets",""])+"\" \"Everything Bundle Installer.pkg\"")
		run_command("xcrun notarytool submit \"Everything Bundle Installer.pkg\" --apple-id "+saved_data["secrets"]["NOTARIZATION_USERNAME"]+" --password "+saved_data["secrets"]["NOTARIZATION_PASSWORD"]+" --team-id "+saved_data["secrets"]["TEAM_ID"]+" --wait")
		run_command("xcrun stapler staple \"Everything Bundle Installer.pkg\"")
		for plugin in plugins:
			for target in targets:
				if plugin["in_bundle"] and (plugin["standalone"] or target["name"] != "Standalone"):
					move(join(["setup","temp",plugin["name"]+target["extension"]]),join(["setup","temp","Manual install",plugin["name"]+target["extension"]]))
			for file in plugin["additional_files"]:
				if (get_system(system_i)["code"]+"_"+("paid" if plugin["paid"] else "free")) in file["versions"] and not file["copy"]:
					copy(join(["plugins",plugin["name"].replace(' ','').lower(),file["path"]]),join(["setup","temp",file["output"]]))
		move("Everything Bundle Installer.pkg",join(["setup","temp","Everything Bundle Installer.pkg"]))
		remove(identifier+".xml")
		for target in targets:
			if target["name"] == "Standalone":
				remove("component.plist")
			remove(identifier+"-"+target["code"].lower()+".pkg")

	elif get_system(system_i)["code"] == "win" and systems[system]["code"] == "win":
		debug("BUILDING INSTALLER FOR EVERYTHING BUNDLE")
		for plugin in plugins:
			if not plugin["in_bundle"]:
				continue
			create_dir(join(["setup","build_win","paid"]))
			create_dir(join(["setup","build_win","free"]))
			for target in targets:
				if target["name"] != "Audio Unit" and (plugin["standalone"] or target["name"] != "Standalone"):
					copy(join(["setup","temp","Manual install",plugin["name"]+target["extension"]]),join(["setup","build_win","paid" if plugin["paid"] else "free",plugin["name"]+target["extension"]]))
			remove(join(["setup","temp",plugin["name"]+" Installer.exe"]))
			for file in plugin["additional_files"]:
				if ("win_"+("paid" if plugin["paid"] else "free")) in file["versions"] and file["copy"]:
					if not os.path.isdir(join(["setup","build_win","other",plugin["name"]])):
						create_dir(join(["setup","build_win","other",plugin["name"]]))
					copy(join(["setup","temp","Manual install",file["output"]]),join(["setup","build_win","other",plugin["name"],file["output"]]))
		run_command("iscc \""+join(["setup","innosetup everything.iss"])+"\"")
		move(join(["setup","Output","Everything Bundle Installer.exe"]),join(["setup","temp","Everything Bundle Installer.exe"]))

	zip_files(join(["setup","temp",""]),join(["setup","zips","Everything Bundle "+get_system(system_i)["name"]+".zip"]),9)


def build_all():
	debug("BUILDING ALL")
	prepare()
	for free in ["free","paid"]:
		configure(free)
		for plugin in plugins:
			if plugin["paid"] or free == "free":
				for target in targets:
					if target["name"] == "Audio Unit" and systems[system]["code"] != "mac":
						continue
					if not plugin["standalone"] and target["name"] == "Standalone":
						continue
					build(plugin["name"],"Release",target["name"])
				build_installer(plugin["name"],system,True)
	build_everything_bundle(system)

def write_actions():
	bundles_done = []
	for plugin in plugins:
		if "bundle" in plugin:
			if plugin["bundle"] in bundles_done:
				continue
			else:
				bundles_done.append(plugin["bundle"])

		path = join([".github","workflows",plugin["name"].replace(' ','').lower()+"_build.yml"])
		debug("WRITING GITHUB ACTIONS WORKFLOW TO "+path)
		to_build = []
		if "bundle" not in plugin:
			to_build.append(plugin)
		else:
			for bundle_plugin in plugins:
				if "bundle" in bundle_plugin and bundle_plugin["bundle"] == plugin["bundle"]:
					to_build.append(bundle_plugin)
		file = open(path,"w")
		file.write("name: "+plugin["name"]+'''

env:
  PLUG: '''+plugin["name"]+'''

on:
  push:
    paths:''')
		for bundle_plugin in to_build:
			file.write('''
      - '**'''+bundle_plugin["name"].replace(' ','').lower()+"**'")
		file.write('''
      - 'rebuild_all.txt'
  pull_request:
    paths:''')
		for bundle_plugin in to_build:
			file.write('''
      - '**'''+bundle_plugin["name"].replace(' ','').lower()+"**'")
		file.write('''
      - 'rebuild_all.txt'
    branches:
      - master

jobs:
  build:
    name: build
    strategy:
      matrix:
        include:''')
		for system_i in systems:
			file.write('''
        - {
            name: "'''+system_i["name"]+'''",
            os: '''+system_i["image"]+''',
            folder: "build_'''+system_i["code"]+'''"
          }''')
		file.write('''
    runs-on: ${{ matrix.os }}
    concurrency: build_${{ matrix.os }}

    steps:
    - uses: actions/checkout@v4

    - name: cache stuff
      id: cache-stuff
      uses: actions/cache@v4
      with:
        path: "${{ matrix.folder }}"
        key: "${{ matrix.folder }}"

    - name: prepare
      id: prepare
      run: |
        python3 build.py prepare
        python3 build.py secrets''')
		for secret in valid_secrets:
			file.write(" "+secret+"=\"${{ secrets."+secret+" }}\"")
		file.write('''

    - name: (mac) import certificates
      id: import-certificates
      if: startsWith(matrix.os, 'mac')
      uses: apple-actions/import-codesign-certs@v2
      with:
        keychain: unplugred
        keychain-password: ${{ secrets.KEYCHAIN_PASSWORD }}
        p12-file-base64: ${{ secrets.DEV_ID_APP_CERT }}
        p12-password: ${{ secrets.DEV_ID_APP_PASSWORD }}

    - name: (mac) import installer certificates
      id: import-installer-certificates
      if: startsWith(matrix.os, 'mac')
      uses: apple-actions/import-codesign-certs@v2
      with:
        keychain: unplugred
        keychain-password: ${{ secrets.KEYCHAIN_PASSWORD }}
        p12-file-base64: ${{ secrets.DEV_ID_INSTALL_CERT }}
        p12-password: ${{ secrets.DEV_ID_INSTALL_PASSWORD }}
        create-keychain: false
''')
		for version in ["paid","free"]:
			if not plugin["paid"] and version == "paid":
				continue
			version_tag = [" free","-free"," Free"] if (plugin["paid"] and version == "free") else ["","",""]
			file.write('''
    - name: configure'''+version_tag[0]+'''
      id: configure'''+version_tag[1]+'''
      run: |
        python3 build.py configure '''+version+'''
''')
			for bundle_plugin in to_build:
				for target in targets:
					if target["name"] == "Standalone" and not bundle_plugin["standalone"]:
						continue
					file.write('''
    - name: build '''+((bundle_plugin["name"].lower()+" ") if "bundle" in plugin else "")+target["name"].lower()+version_tag[0]+'''
      id: build-'''+((bundle_plugin["name"].lower()+"-") if "bundle" in plugin else "")+target["code"].lower()+version_tag[1])
					if target["name"] == "Audio Unit":
						file.write('''
      if: startsWith(matrix.os, 'mac')''')
					file.write('''
      run: |
        python3 build.py "'''+(bundle_plugin["name"] if "bundle" in plugin else "${{ env.PLUG }}")+'''" release '''+target["code"].lower()+(" no" if target["name"] == "Standalone" else "")+'''
''')
			file.write('''
    - name: installer'''+version_tag[0]+'''
      id: installer'''+version_tag[1]+'''
      run: |
        python3 build.py "${{ env.PLUG }}" installer

    - uses: actions/upload-artifact@v4
      with:
        name: ${{ env.PLUG }}'''+version_tag[2]+''' ${{ matrix.name }}
        path: setup/temp
        compression-level: 9
''')
		file.write('''
    - name: cleanup
      id: cleanup
      run: |
        rm -r setup/temp
        rm saved_data.json''')
		file.close()

def run_program(string):
	if string.strip() == "":
		error("You must specify arguments!")
	args = shlex.split(string)

	if "configure".startswith(args[0]) and ',' not in string and len(args) <= 2:
		match = "free"
		if len(args) == 2:
			match = fuzzy_match(args[1],codes["version"])
			if match == None:
				error("Unknown version: "+args[1])
		configure(match)
		return

	to_build = {}
	arguments = ["plugin","config","target","run","system"]
	for argument in arguments:
		to_build[argument] = []
	installer = False
	everything = False
	for i in range(len(args)):
		if i > 4 or (i > 3 and ("Standalone" not in to_build["target"] or not installer)) or (i > 2 and "Standalone" not in to_build["target"] and not installer):
			error("Unexpected number of arguments!")
		if (i == 0 and "everythingbundle".startswith(args[i])) or (i == 2 and "all".startswith(args[i])):
			if i == 0:
				everything = True
			for n in codes[arguments[i]]:
				to_build[arguments[i]].append(n)
			continue
		if i == 1 and "installer".startswith(args[i]) and ',' not in args[i]:
			installer = True
			continue
		if i == 3 and ',' in args[i]:
			error("Argument "+arguments[i]+" can only have one value")
		sub_args = args[i].split(',')
		for n in range(len(sub_args)):
			if i == 2 and "installer".startswith(sub_args[n]):
				if installer:
					error("Argument Installer used more than once!")
				installer = True
				continue
			match = fuzzy_match(sub_args[n],codes[arguments[i]])
			if match == None:
				if i == 0 and ',' not in args[i]:
					if len(args) == 1:
						if "prepare".startswith(sub_args[n]):
							prepare()
							return
						elif "actions".startswith(sub_args[n]):
							write_actions()
							return
					elif "secrets".startswith(sub_args[n]) and len(args) > 1:
						update_secrets(args[1:])
						return
				if installer:
					match = fuzzy_match(sub_args[n],codes["system"])
					if match == None:
						error("Unknown system: "+sub_args[n])
					if match in to_build["system"]:
						error("Argument "+match+" used more than once!")
					to_build["system"].append(match)
					continue
				error("Unknown "+arguments[i]+": "+sub_args[n])
			if match in to_build[arguments[i]]:
				error("Argument "+match+" used more than once!")
			to_build[arguments[i]].append(match)
	if installer and len(to_build["target"]) == 0 and len(to_build["config"]) > 0:
		error("Cannot use config "+to_build["config"][0]+" to build an installer")

	if len(to_build["system"]) == 0:
		if len(to_build["target"]) == 0 and everything:
			for system_i in systems:
				to_build["system"].append(system_i["name"])
		else:
			to_build["system"].append(systems[system]["name"])
	if len(to_build["config"]) == 0:
		to_build["config"].append("Debug")
	if len(to_build["target"]) == 0 and not installer:
		to_build["target"].append("Standalone")
	if len(to_build["run"]) == 0:
		to_build["run"].append("yes" if "Standalone" in to_build["target"] else "no")

	if installer and len(to_build["target"]) == 0:
		if everything:
			for system_i in to_build["system"]:
				build_everything_bundle(system_i)
		else:
			for plugin in to_build["plugin"]:
				for system_i in to_build["system"]:
					build_installer(plugin,system_i)
		return

	for plugin in to_build["plugin"]:
		for config in to_build["config"]:
			for target in to_build["target"]:
				build(plugin,config,target)
			if to_build["run"][0] == "yes":
				run_plugin(plugin,config)
		if installer:
			for system_i in to_build["system"]:
				build_installer(plugin,system_i)
	if everything and installer:
		for system_i in to_build["system"]:
			build_everything_bundle(system_i)

if __name__ == "__main__":
	run_program(' '.join(shlex.quote(s) for s in (sys.argv[1:])))
