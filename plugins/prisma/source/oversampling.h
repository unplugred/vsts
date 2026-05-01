// modified version of juce's dsp::oversampling implementation (gpl3)

#pragma once
#include "includes.h"

template<typename SampleType> class JUCE_API Oversampling {
public:
	explicit Oversampling(size_t numChannels = 1);
	Oversampling(size_t numChannels, size_t maxFactor, bool isMaxQuality = true, bool useIntegerLatency = false);
	~Oversampling();
	void setUsingIntegerLatency(bool shouldUseIntegerLatency) noexcept;
	SampleType getLatencyInSamples() const noexcept;
	void setOversamplingFactor(size_t newfactor) noexcept;
	void initProcessing(size_t maximumNumberOfSamplesBeforeOversampling, size_t newfactor);
	void reset() noexcept;
	void processSamplesUp  (dsp::AudioBlock<SampleType>& inputBlock, dsp::AudioBlock<SampleType>& outputBlock) noexcept;
	void processSamplesDown(dsp::AudioBlock<SampleType>& inputBlock, dsp::AudioBlock<SampleType>& outputBlock) noexcept;
	void addOversamplingStage(
			float normalisedTransitionWidthUp  ,float stopbandAmplitudedBUp  ,
			float normalisedTransitionWidthDown,float stopbandAmplitudedBDown);
	void addDummyOversamplingStage();
	void clearOversamplingStages();

	size_t factor = 1;
	size_t numChannels = 1;

	struct OversamplingStage;

private:
	void updateDelayLine();
	SampleType getUncompensatedLatency() const noexcept;

	OwnedArray<OversamplingStage> stages;
	bool isReady = false;
	bool shouldUseIntegerLatency = false;
	dsp::DelayLine<SampleType,dsp::DelayLineInterpolationTypes::Thiran> delay{8};
	SampleType fractionalDelay = 0;

	AudioBuffer<SampleType> tempbuffer;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Oversampling)
};
