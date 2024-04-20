def execute():
	import sys

	pluginname = ""
	version = ""
	nopaid = False
	standalone = False

	for i in range(len(sys.argv)):
		if sys.argv[i] == "--pluginname":
			pluginname = sys.argv[i+1]
		elif sys.argv[i] == "--version":
			version = sys.argv[i+1]
		elif sys.argv[i] == "--nopaid":
			nopaid = True
		elif sys.argv[i] == "--standalone":
			standalone = True
	if pluginname == "":
		print("No --pluginname argument provided")
		return
	if pluginname == "" or version == "":
		print("No --version argument provided")
		return

	if nopaid or version == "paid":
		version_tag = ""
		file = open(pluginname+".xml","w")
	else:
		version_tag = "-free"
		file = open(pluginname+" free.xml","w")
	formats = [{
			"id": "au",
			"name": "Audio Unit",
			"description": "For Logic and GarageBand"
		},{
			"id": "clap",
			"name": "CLAP",
			"description": "For Reaper and Bitwig (currently)"
		},{
			"id": "vst3",
			"name": "VST3",
			"description": "For everything else"
	}]
	if standalone:
		formats.append({
			"id": "standalone",
			"name": "Standalone",
			"description": "For you. No DAW required"
		})

	file.write('''<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
	<title>'''+pluginname+''' Install</title>
	<domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
	<allowed-os-versions>
		<os-version min="10.13"/>
	</allowed-os-versions>
	<options customize="always" require-scripts="false" rootVolumeOnly="true"/>
	<welcome file="pages/welcome.html" mime-type="text/html"/>
	<conclusion file="pages/conclusion.html" mime-type="text/html"/>
	<choices-outline>''')
	for e in formats:
		file.write('''
		<line choice="'''+e['id']+'''"/>''')
	file.write('''
	</choices-outline>''')
	for e in formats:
		file.write('''
	<choice id="'''+e['id']+'" title="'+e['name']+'" description="'+e['description']+'''">
		<pkg-ref id="com.unplugred.'''+pluginname.replace(" ","")+version_tag+'-'+e['id']+'">'+pluginname.replace(" ","")+version_tag+'-'+e['id']+'''.pkg</pkg-ref>
	</choice>''')
	file.write('''
</installer-gui-script>
''')

	file.close()
execute()
