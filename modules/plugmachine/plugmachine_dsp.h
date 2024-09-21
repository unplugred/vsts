/* dsp engine for my plugins  */

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
using namespace juce;

#define CONSOLE_LENGTH 20

class plugmachine_dsp : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
public:
	void init();
	void close();

	plugmachine_dsp(const BusesProperties& io_layouts, AudioProcessorValueTreeState* apvts);
	~plugmachine_dsp() override;

	virtual const String get_preset(int preset_id, const char delimiter = ',');
	virtual void set_preset(const String& preset, int preset_id, const char delimiter = ',', bool print_errors = false);
	virtual bool is_valid_preset_string(String preset, const char delimiter = ',');

	AudioProcessorValueTreeState* apvts_ref;
	void add_listener(String name);

	double ui_scale = -1;
	ApplicationProperties props;
	void set_ui_scale(double new_ui_scale);

	UndoManager undo_manager;

	void debug(String str, bool timestamp = true);
	void debug(int str, bool timestamp = true);
	void debug(float str, bool timestamp = true);
	void debug(double str, bool timestamp = true);
	void debug(bool str, bool timestamp = true);
	std::mutex debug_mutex;
	String debug_text = "";

private:
	String debug_list[CONSOLE_LENGTH];
	int debug_read_pos = 0;

	std::vector<String> listeners;
};
