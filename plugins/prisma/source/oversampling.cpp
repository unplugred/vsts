#include "oversampling.h"

template<typename SampleType> struct Oversampling<SampleType>::OversamplingStage {
	OversamplingStage(size_t numChans) : numChannels(numChans) {}
	virtual ~OversamplingStage() {}

	virtual SampleType getLatencyInSamples() const = 0;
	virtual void reset() = 0;
	virtual void processSamplesUp  (dsp::AudioBlock<SampleType>&, dsp::AudioBlock<SampleType>&) = 0;
	virtual void processSamplesDown(dsp::AudioBlock<SampleType>&, dsp::AudioBlock<SampleType>&) = 0;

	size_t numChannels;
};


template<typename SampleType> struct Oversampling2TimesPolyphaseIIR final : public Oversampling<SampleType>::OversamplingStage {
	using ParentType = typename Oversampling<SampleType>::OversamplingStage;

	Oversampling2TimesPolyphaseIIR(size_t numChans,
			SampleType normalisedTransitionWidthUp,
			SampleType stopbandAmplitudedBUp,
			SampleType normalisedTransitionWidthDown,
			SampleType stopbandAmplitudedBDown) : ParentType(numChans) {

		auto structureUp   = dsp::FilterDesign<SampleType>::designIIRLowpassHalfBandPolyphaseAllpassMethod(normalisedTransitionWidthUp  ,stopbandAmplitudedBUp  );
		auto structureDown = dsp::FilterDesign<SampleType>::designIIRLowpassHalfBandPolyphaseAllpassMethod(normalisedTransitionWidthDown,stopbandAmplitudedBDown);
		auto coeffsUp   = getCoefficients(structureUp  );
		auto coeffsDown = getCoefficients(structureDown);
		latency  = static_cast<SampleType>(-(coeffsUp  .getPhaseForFrequency(.0001,1.))/(.0001*MathConstants<double>::twoPi));
		latency += static_cast<SampleType>(-(coeffsDown.getPhaseForFrequency(.0001,1.))/(.0001*MathConstants<double>::twoPi));

		for(auto i = 0; i < structureUp  .directPath .size(); ++i) coefficientsUp  .add(structureUp  .directPath .getObjectPointer(i)->coefficients[0]);
		for(auto i = 1; i < structureUp  .delayedPath.size(); ++i) coefficientsUp  .add(structureUp  .delayedPath.getObjectPointer(i)->coefficients[0]);
		for(auto i = 0; i < structureDown.directPath .size(); ++i) coefficientsDown.add(structureDown.directPath .getObjectPointer(i)->coefficients[0]);
		for(auto i = 1; i < structureDown.delayedPath.size(); ++i) coefficientsDown.add(structureDown.delayedPath.getObjectPointer(i)->coefficients[0]);

		v1Up    .setSize(static_cast<int>(this->numChannels),coefficientsUp  .size());
		v1Down  .setSize(static_cast<int>(this->numChannels),coefficientsDown.size());
		delayDown.resize(static_cast<int>(this->numChannels));
	}

	SampleType getLatencyInSamples() const override {
		return latency;
	}

	void reset() override {
		v1Up.clear();
		v1Down.clear();
		delayDown.fill(0);
	}

	void processSamplesUp(dsp::AudioBlock<SampleType>& inputBlock, dsp::AudioBlock<SampleType>& outputBlock) override {
		jassert(inputBlock.getNumChannels()<=outputBlock.getNumChannels());
		jassert((inputBlock.getNumSamples()*2)<=outputBlock.getNumSamples());

		auto coeffs = coefficientsUp.getRawDataPointer();
		auto numStages = coefficientsUp.size();
		auto delayedStages = numStages/2;
		auto directStages = numStages-delayedStages;
		auto numSamples = inputBlock.getNumSamples();

		for(size_t channel = 0; channel < inputBlock.getNumChannels(); ++channel) {
			auto bufferSamples = outputBlock.getChannelPointer(channel);
			auto lv1 = v1Up.getWritePointer(static_cast<int>(channel));
			auto samples = inputBlock.getChannelPointer(channel);

			for(size_t i = 0; i < numSamples; ++i) {

				auto input = samples[i];
				for(auto n = 0; n < directStages; ++n) {
					auto alpha = coeffs[n];
					auto output = alpha*input+lv1[n];
					lv1[n] = input-alpha*output;
					input = output;
				}
				bufferSamples[i<<1] = input;

				input = samples[i];
				for(auto n = directStages; n < numStages; ++n) {
					auto alpha = coeffs[n];
					auto output = alpha*input+lv1[n];
					lv1[n] = input-alpha*output;
					input = output;
				}
				bufferSamples[(i<<1)+1] = input;
			}
		}

#if JUCE_DSP_ENABLE_SNAP_TO_ZERO
		snapToZero(true);
#endif
	}

	void processSamplesDown(dsp::AudioBlock<SampleType>& inputBlock, dsp::AudioBlock<SampleType>& outputBlock) override {
		jassert(outputBlock.getNumChannels()<=inputBlock.getNumChannels());
		jassert(outputBlock.getNumSamples()*2<=inputBlock.getNumSamples());

		// Initialization
		auto coeffs = coefficientsDown.getRawDataPointer();
		auto numStages = coefficientsDown.size();
		auto delayedStages = numStages/2;
		auto directStages = numStages-delayedStages;
		auto numSamples = outputBlock.getNumSamples();

		// Processing
		for(size_t channel = 0; channel < outputBlock.getNumChannels(); ++channel) {
			auto bufferSamples = inputBlock.getChannelPointer(channel);
			auto lv1 = v1Down.getWritePointer(static_cast<int>(channel));
			auto samples = outputBlock.getChannelPointer(channel);
			auto delay = delayDown.getUnchecked(static_cast<int>(channel));

			for(size_t i = 0; i < numSamples; ++i) {

				auto input = bufferSamples[i<<1];
				for(auto n = 0; n < directStages; ++n) {
					auto alpha = coeffs[n];
					auto output = alpha*input+lv1[n];
					lv1[n] = input-alpha*output;
					input = output;
				}
				auto directOut = input;

				input = bufferSamples[(i<<1)+1];
				for(auto n = directStages; n < numStages; ++n) {
					auto alpha = coeffs[n];
					auto output = alpha*input+lv1[n];
					lv1[n] = input-alpha*output;
					input = output;
				}

				samples[i] = (delay+directOut)*static_cast<SampleType>(.5);
				delay = input;
			}

			delayDown.setUnchecked(static_cast<int>(channel),delay);
		}

#if JUCE_DSP_ENABLE_SNAP_TO_ZERO
		snapToZero(false);
#endif
	}

	void snapToZero(bool snapUpProcessing) {
		if(snapUpProcessing) {
			for(auto channel = 0; channel < ParentType::numChannels; ++channel) {
				auto lv1 = v1Up.getWritePointer(channel);
				auto numStages = coefficientsUp.size();

				for(auto n = 0; n < numStages; ++n)
					dsp::util::snapToZero(lv1[n]);
			}
		} else {
			for(auto channel = 0; channel < ParentType::numChannels; ++channel) {
				auto lv1 = v1Down.getWritePointer(channel);
				auto numStages = coefficientsDown.size();

				for(auto n = 0; n < numStages; ++n)
					dsp::util::snapToZero(lv1[n]);
			}
		}
	}

private:
	dsp::IIR::Coefficients<SampleType> getCoefficients(typename dsp::FilterDesign<SampleType>::IIRPolyphaseAllpassStructure& structure) const {
		constexpr auto one = static_cast<SampleType>(1.);

		dsp::Polynomial<SampleType> numerator1({one}),denominator1({one}),
		                       numerator2({one}),denominator2({one});

		for(auto* i : structure.directPath) {
			auto coeffs = i->getRawCoefficients();

			if(i->getFilterOrder() == 1) {
				numerator1	 = numerator1  .getProductWith(dsp::Polynomial<SampleType>({coeffs[0],coeffs[1]}));
				denominator1 = denominator1.getProductWith(dsp::Polynomial<SampleType>({one      ,coeffs[2]}));
			} else {
				numerator1   = numerator1  .getProductWith(dsp::Polynomial<SampleType>({coeffs[0],coeffs[1],coeffs[2]}));
				denominator1 = denominator1.getProductWith(dsp::Polynomial<SampleType>({one      ,coeffs[3],coeffs[4]}));
			}
		}

		for(auto* i : structure.delayedPath) {
			auto coeffs = i->getRawCoefficients();

			if(i->getFilterOrder() == 1) {
				numerator2   = numerator2  .getProductWith(dsp::Polynomial<SampleType>({coeffs[0],coeffs[1]}));
				denominator2 = denominator2.getProductWith(dsp::Polynomial<SampleType>({one      ,coeffs[2]}));
			} else {
				numerator2	 = numerator2  .getProductWith(dsp::Polynomial<SampleType>({coeffs[0],coeffs[1],coeffs[2]}));
				denominator2 = denominator2.getProductWith(dsp::Polynomial<SampleType>({one      ,coeffs[3],coeffs[4]}));
			}
		}

		auto numeratorf1 = numerator1.getProductWith(denominator2);
		auto numeratorf2 = numerator2.getProductWith(denominator1);
		auto numerator   = numeratorf1.getSumWith(numeratorf2);
		auto denominator = denominator1.getProductWith(denominator2);

		dsp::IIR::Coefficients<SampleType> coeffs;

		coeffs.coefficients.clear();
		auto inversion = one/denominator[0];

		for(int i = 0; i <= numerator.getOrder(); ++i)
			coeffs.coefficients.add(numerator[i]*inversion);

		for(int i = 1; i <= denominator.getOrder(); ++i)
			coeffs.coefficients.add(denominator[i]*inversion);

		return coeffs;
	}

	Array<SampleType> coefficientsUp, coefficientsDown;
	SampleType latency;

	AudioBuffer<SampleType> v1Up, v1Down;
	Array<SampleType> delayDown;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Oversampling2TimesPolyphaseIIR)
};


template<typename SampleType> Oversampling<SampleType>::Oversampling(size_t newNumChannels) : numChannels(newNumChannels) {
	jassert(numChannels > 0);
}

template<typename SampleType> Oversampling<SampleType>::Oversampling(size_t newNumChannels, size_t maxFactor, bool isMaximumQuality, bool useIntegerLatency) : numChannels(newNumChannels), shouldUseIntegerLatency(useIntegerLatency) {
	jassert(isPositiveAndBelow(maxFactor,5)&&numChannels>0);

	for(size_t n = 0; n < maxFactor; ++n) {
		auto twUp   = (isMaximumQuality?.10f:.12f)*(n==0?.5f:1.f);
		auto twDown = (isMaximumQuality?.12f:.15f)*(n==0?.5f:1.f);

		auto gaindBStartUp	  = (isMaximumQuality?-90.f:-70.f);
		auto gaindBStartDown  = (isMaximumQuality?-75.f:-60.f);
		auto gaindBFactorUp   = (isMaximumQuality? 10.f:  8.f);
		auto gaindBFactorDown = (isMaximumQuality? 10.f:  8.f);

		addOversamplingStage(twUp  ,gaindBStartUp  +gaindBFactorUp  *(float)n,
		                     twDown,gaindBStartDown+gaindBFactorDown*(float)n);
	}
}

template<typename SampleType> Oversampling<SampleType>::~Oversampling() {
	stages.clear();
}

template<typename SampleType> void Oversampling<SampleType>::addOversamplingStage(float normalisedTransitionWidthUp, float stopbandAmplitudedBUp, float normalisedTransitionWidthDown, float stopbandAmplitudedBDown) {
	stages.add(new Oversampling2TimesPolyphaseIIR<SampleType>(numChannels,
		normalisedTransitionWidthUp  ,stopbandAmplitudedBUp  ,
		normalisedTransitionWidthDown,stopbandAmplitudedBDown));
}

template<typename SampleType> void Oversampling<SampleType>::clearOversamplingStages() {
	stages.clear();
}

template<typename SampleType> void Oversampling<SampleType>::setUsingIntegerLatency(bool useIntegerLatency) noexcept {
	shouldUseIntegerLatency = useIntegerLatency;
}

template<typename SampleType> SampleType Oversampling<SampleType>::getLatencyInSamples() const noexcept {
	auto latency = getUncompensatedLatency();
	return shouldUseIntegerLatency?latency+fractionalDelay:latency;
}

template<typename SampleType> SampleType Oversampling<SampleType>::getUncompensatedLatency() const noexcept {
	auto latency = static_cast<SampleType>(0);
	size_t order = 1;

	for(int i = 0; i < factor; ++i) {
		order *= 2;
		latency += stages[i]->getLatencyInSamples()/static_cast<SampleType>(order);
	}

	return latency;
}

template<typename SampleType> void Oversampling<SampleType>::setOversamplingFactor(size_t newfactor) noexcept {
	factor = newfactor;
	updateDelayLine();
}

template<typename SampleType> void Oversampling<SampleType>::initProcessing(size_t maximumNumberOfSamplesBeforeOversampling, size_t newfactor) {
	jassert(!stages.isEmpty());

	if(stages.size() > 1)
		tempbuffer.setSize(static_cast<int>(numChannels),static_cast<int>(maximumNumberOfSamplesBeforeOversampling*pow(2,stages.size()-1)),false,false,true);

	dsp::ProcessSpec spec = {0.,(uint32)maximumNumberOfSamplesBeforeOversampling,(uint32)numChannels};
	delay.prepare(spec);

	setOversamplingFactor(newfactor);

	isReady = true;
	reset();
}

template<typename SampleType> void Oversampling<SampleType>::reset() noexcept {
	jassert(!stages.isEmpty());

	if(isReady)
		for(auto* stage : stages)
			stage->reset();

	delay.reset();
}

template<typename SampleType> void Oversampling<SampleType>::processSamplesUp(dsp::AudioBlock<SampleType>& inputBlock, dsp::AudioBlock<SampleType>& outputBlock) noexcept {
	jassert(!stages.isEmpty());
	if(!isReady) return;
	if(factor == 0) return;

	auto* firstStage = stages.getUnchecked(0);
	if(factor == 1) {
		firstStage->processSamplesUp(inputBlock,outputBlock);
	} else {
		bool istempblock = (factor%2)==0;
		dsp::AudioBlock<SampleType> tempblock(tempbuffer);
		size_t cfactor = 2;
		if(istempblock) {
			dsp::AudioBlock<SampleType> outsubblock = tempblock  .getSubBlock(0,inputBlock.getNumSamples()*cfactor);
			firstStage->processSamplesUp(inputBlock,outsubblock);
		} else {
			dsp::AudioBlock<SampleType> outsubblock = outputBlock.getSubBlock(0,inputBlock.getNumSamples()*cfactor);
			firstStage->processSamplesUp(inputBlock,outsubblock);
		}
		for(int i = 1; i < factor; ++i) {
			istempblock = !istempblock;
			if(istempblock) {
				dsp::AudioBlock<SampleType>  insubblock = outputBlock.getSubBlock(0,inputBlock.getNumSamples()*cfactor  );
				dsp::AudioBlock<SampleType> outsubblock = tempblock  .getSubBlock(0,inputBlock.getNumSamples()*cfactor*2);
				stages[i]->processSamplesUp(insubblock,outsubblock);
			} else {
				dsp::AudioBlock<SampleType>  insubblock = tempblock  .getSubBlock(0,inputBlock.getNumSamples()*cfactor  );
				dsp::AudioBlock<SampleType> outsubblock = outputBlock.getSubBlock(0,inputBlock.getNumSamples()*cfactor*2);
				stages[i]->processSamplesUp(insubblock,outsubblock);
			}
			cfactor *= 2;
		}
	}
}

template<typename SampleType> void Oversampling<SampleType>::processSamplesDown(dsp::AudioBlock<SampleType>& inputBlock, dsp::AudioBlock<SampleType>& outputBlock) noexcept {
	jassert(!stages.isEmpty());
	if(!isReady) return;
	if(factor == 0) return;

	if(factor == 1) {
		stages.getFirst()->processSamplesDown(inputBlock,outputBlock);
	} else {
		bool istempblock = (factor%2)==0;
		dsp::AudioBlock<SampleType> tempblock(tempbuffer);
		size_t cfactor = 1;
		for(int n = (factor-1); n > 0; --n) {
			auto& stage = *stages.getUnchecked(n);
			if(istempblock) {
				dsp::AudioBlock<SampleType>  insubblock = inputBlock.getSubBlock(0,inputBlock.getNumSamples()/(cfactor*2));
				dsp::AudioBlock<SampleType> outsubblock = tempblock .getSubBlock(0,inputBlock.getNumSamples()/ cfactor   );
				stage.processSamplesDown(insubblock,outsubblock);
			} else {
				dsp::AudioBlock<SampleType>  insubblock = tempblock .getSubBlock(0,inputBlock.getNumSamples()/(cfactor*2));
				dsp::AudioBlock<SampleType> outsubblock = inputBlock.getSubBlock(0,inputBlock.getNumSamples()/ cfactor   );
				stage.processSamplesDown(insubblock,outsubblock);
			}
			istempblock = !istempblock;
			cfactor *= 2;
		}
		if(istempblock) {
			dsp::AudioBlock<SampleType> insubblock = inputBlock.getSubBlock(0,inputBlock.getNumSamples()/(cfactor*2));
			stages.getFirst()->processSamplesDown(insubblock,outputBlock);
		} else {
			dsp::AudioBlock<SampleType> insubblock = tempblock .getSubBlock(0,inputBlock.getNumSamples()/(cfactor*2));
			stages.getFirst()->processSamplesDown(insubblock,outputBlock);
		}
	}

	if(shouldUseIntegerLatency && fractionalDelay > static_cast<SampleType>(0.)) {
		auto context = dsp::ProcessContextReplacing<SampleType>(outputBlock);
		delay.process(context);
	}
}

template<typename SampleType> void Oversampling<SampleType>::updateDelayLine() {
	auto latency = getUncompensatedLatency();
	fractionalDelay = static_cast<SampleType>(1.)-(latency-std::floor(latency));

	if(approximatelyEqual(fractionalDelay,static_cast<SampleType>(1.)))
		fractionalDelay  = static_cast<SampleType>(0.);
	else if(fractionalDelay < static_cast<SampleType>(.618))
		fractionalDelay += static_cast<SampleType>(1.);

	delay.setDelay(fractionalDelay);
}

template class Oversampling<float >;
template class Oversampling<double>;
