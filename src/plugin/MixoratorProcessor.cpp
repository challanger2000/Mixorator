#include "MixoratorProcessor.h"
#include "MixoratorIDs.h"

namespace Mixorator
{
Processor::Processor()
{
    setControllerClass(kControllerUID);
}

Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context)
{
    const auto result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultOk)
        return result;

    addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Processor::setupProcessing(Steinberg::Vst::ProcessSetup& setup)
{
    const auto result = AudioEffect::setupProcessing(setup);
    if (result != Steinberg::kResultOk)
        return result;

    analysis_.prepare(setup.sampleRate);
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool state)
{
    if (state)
        analysis_.reset();

    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API Processor::setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs,
    Steinberg::int32 numIns,
    Steinberg::Vst::SpeakerArrangement* outputs,
    Steinberg::int32 numOuts)
{
    if (numIns == 1 && numOuts == 1 &&
        inputs[0] == Steinberg::Vst::SpeakerArr::kStereo &&
        outputs[0] == Steinberg::Vst::SpeakerArr::kStereo)
    {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }

    return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(Steinberg::int32 symbolicSampleSize)
{
    return (symbolicSampleSize == Steinberg::Vst::kSample32 ||
            symbolicSampleSize == Steinberg::Vst::kSample64)
               ? Steinberg::kResultTrue
               : Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data)
{
    if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples <= 0)
        return Steinberg::kResultOk;

    auto& input = data.inputs[0];
    auto& output = data.outputs[0];
    const auto channels = input.numChannels < output.numChannels ? input.numChannels : output.numChannels;

    if (data.symbolicSampleSize == Steinberg::Vst::kSample32)
    {
        analysis_.process(input.channelBuffers32, input.numChannels, data.numSamples);

        for (Steinberg::int32 ch = 0; ch < channels; ++ch)
        {
            const auto* in = input.channelBuffers32[ch];
            auto* out = output.channelBuffers32[ch];
            if (!in || !out || in == out)
                continue;

            for (Steinberg::int32 i = 0; i < data.numSamples; ++i)
                out[i] = in[i];
        }
    }
    else if (data.symbolicSampleSize == Steinberg::Vst::kSample64)
    {
        analysis_.process(input.channelBuffers64, input.numChannels, data.numSamples);

        for (Steinberg::int32 ch = 0; ch < channels; ++ch)
        {
            const auto* in = input.channelBuffers64[ch];
            auto* out = output.channelBuffers64[ch];
            if (!in || !out || in == out)
                continue;

            for (Steinberg::int32 i = 0; i < data.numSamples; ++i)
                out[i] = in[i];
        }
    }

    output.silenceFlags = input.silenceFlags;
    return Steinberg::kResultOk;
}
}
