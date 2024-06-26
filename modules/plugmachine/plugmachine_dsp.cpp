#include "plugmachine_dsp.h"

plugmachine_dsp::plugmachine_dsp(const BusesProperties& io_layouts, AudioProcessorValueTreeState* apvts) :
	AudioProcessor(io_layouts) {

	listeners.clear();
	apvts_ref = apvts;
}
plugmachine_dsp::~plugmachine_dsp() {
}

const String plugmachine_dsp::get_preset(int preset_id, const char delimiter) {
	return "[NOT IMPLEMENTED]";
}
void plugmachine_dsp::set_preset(const String& preset, int preset_id, const char delimiter, bool print_errors) {
}
bool plugmachine_dsp::is_valid_preset_string(String preset, const char delimiter) {
	String trimstring = preset.trim();
	if(trimstring.isEmpty())
		return false;
	if(!trimstring.containsOnly(String(&delimiter,1)+"-.0123456789"))
		return false;
	if(!trimstring.endsWithChar(delimiter))
		return false;
	if(trimstring.startsWithChar(delimiter))
		return false;
	return true;
}


void plugmachine_dsp::init() {
	PropertiesFile::Options options;
	options.applicationName = getName();
	options.filenameSuffix = ".settings";
	options.osxLibrarySubFolder = "Application Support";
	options.folderName = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile(getName()).getFullPathName();
	options.storageFormat = PropertiesFile::storeAsXML;
	props.setStorageParameters(options);
	PropertiesFile* user_settings = props.getUserSettings();
	if(user_settings->containsKey("UIScale"))
		ui_scale = props.getUserSettings()->getDoubleValue("UIScale");
}
void plugmachine_dsp::set_ui_scale(double new_ui_scale) {
	ui_scale = new_ui_scale;
	props.getUserSettings()->setValue("UIScale",ui_scale);
}

void plugmachine_dsp::add_listener(String name) {
	apvts_ref->addParameterListener(name, this);
	listeners.push_back(name);
}
void plugmachine_dsp::close() {
	for(int i = 0; i < listeners.size(); ++i)
		apvts_ref->removeParameterListener(listeners[i], this);
}

void plugmachine_dsp::debug(String str, bool timestamp) {
	DBG(str);

	if(timestamp)
		debug_list[debug_read_pos] = Time::getCurrentTime().toString(false,true,true,true) + " " + str;
	else
		debug_list[debug_read_pos] = str;
	debug_read_pos = fmod(debug_read_pos+1,CONSOLE_LENGTH);

	std::ostringstream console;
	for (int i = 0; i < CONSOLE_LENGTH; i++) {
		console << debug_list[(int)fmod(i+debug_read_pos,CONSOLE_LENGTH)].toStdString();
		if(i < (CONSOLE_LENGTH-1)) console << "\n";
	}
	std::lock_guard<std::mutex> guard(debug_mutex);
	debug_text = (String)console.str();
}
void plugmachine_dsp::debug(int str, bool timestamp) {
	debug((String)str,timestamp);
}
void plugmachine_dsp::debug(float str, bool timestamp) {
	debug((String)str,timestamp);
}
void plugmachine_dsp::debug(double str, bool timestamp) {
	debug((String)str,timestamp);
}
void plugmachine_dsp::debug(bool str, bool timestamp) {
	debug((String)(str?"true":"false"),timestamp);
}