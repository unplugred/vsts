def execute():
	import sys

	pluginname = ""
	version = ""
	nopaid = False
	standalone = False

	for i in range(len(sys.argv)):
		match sys.argv[i]:
			case "--pluginname":
				pluginname = sys.argv[i+1]
			case "--version":
				version = sys.argv[i+1]
			case "--nopaid":
				nopaid = True
			case "--standalone":
				standalone = True
	if pluginname == "" or version == "":
		return

	if nopaid or version == "paid":
		version_tag = ""
	else:
		version_tag = "-free"
	file = open(pluginname+version_tag+".xml","w")

	file.write("""<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
	<title>"""+pluginname+""" Install</title>
	<domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
	<allowed-os-versions>
		<os-version min="10.13"/>
	</allowed-os-versions>
	<options customize="always" require-scripts="false" rootVolumeOnly="true"/>
	<welcome file="pages/welcome.html" mime-type="text/html"/>
	<conclusion file="pages/conclusion.html" mime-type="text/html"/>
	<choices-outline>
		<line choice="au"/>
		<line choice="vst3"/>
		<line choice="clap"/>
	</choices-outline>
	<choice id="au" title="Audio Unit" description="For Logic and Garageband">
		<pkg-ref id="com.unplugred."""+pluginname.lower()+version_tag+"""-au"  >"""+pluginname.lower()+version_tag+"""-au.pkg</pkg-ref>
	</choice>
	<choice id="clap" title="CLAP" description="For Reaper and Bitwig">
		<pkg-ref id="com.unplugred."""+pluginname.lower()+version_tag+"""-clap">"""+pluginname.lower()+version_tag+"""-clap.pkg</pkg-ref>
	</choice>
	<choice id="vst3" title="VST3" description="For everything else">
		<pkg-ref id="com.unplugred."""+pluginname.lower()+version_tag+"""-vst3">"""+pluginname.lower()+version_tag+"""-vst3.pkg</pkg-ref>
	</choice>""")
	if standalone:
		file.write("""
	<choice id="standalone" title="Standalone" description="For you.">
		<pkg-ref id="com.unplugred."""+pluginname.lower()+version_tag+"""-standalone">"""+pluginname.lower()+version_tag+"""-standalone.pkg</pkg-ref>
	</choice>""")
	file.write("""
</installer-gui-script>
""")

	file.close()
execute()
