#include "MixoratorProcessor.h"

#include "pluginterfaces/vst/ivstparameterchanges.h"

namespace Mixorator
{
Processor::Processor()
{
    setControllerClass(Steinberg::FUID(0, 0, 0, 0));
}

Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context)
{
    auto result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultOk)
        return result;

    addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

    return Steinberg::kResultOk;
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

Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data)
{
    if (data.numInputs == 0 || data.numOutputs == 0)
        return Steinberg::kResultOk;

    auto& input = data.inputs[0];
    auto& output = data.outputs[0];

    if (data.symbolicSampleSize == Steinberg::Vst::kSample32)
    {
        for (Steinberg::int32 ch = 0; ch < input.numChannels && ch < output.numChannels; ++ch)
        {
            auto* in = input.channelBuffers32[ch];
            auto* out = output.channelBuffers32[ch];
            if (in != out)
            {
                for (Steinberg::int32 i = 0; i < data.numSamples; ++i)
                    out[i] = in[i];
            }
        }
    }
    else if (data.symbolicSampleSize == Steinberg::Vst::kSample64)
    {
        for (Steinberg::int32 ch = 0; ch < input.numChannels && ch < output.numChannels; ++ch)
        {
            auto* in = input.channelBuffers64[ch];
            auto* out = output.channelBuffers64[ch];
            if (in != out)
            {
                for (Steinberg::int32 i = 0; i < data.numSamples; ++i)
                    out[i] = in[i];
            }
        }
    }

    return Steinberg::kResultOk;
}
}
