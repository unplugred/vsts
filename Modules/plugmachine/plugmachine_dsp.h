/* dsp engine for my plugins  */

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <cool_logger/cool_logger.h>
using namespace juce;

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

	double ui_scale = 1;
	ApplicationProperties props;
	void set_ui_scale(double new_ui_scale);

	CoolLogger logger;
	UndoManager undo_manager;

private:
	std::vector<String> listeners;
};
