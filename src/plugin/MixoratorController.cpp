#include "MixoratorController.h"

#include <cstring>

namespace Mixorator
{
Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context)
{
    hasPacket_ = false;
    latestPacket_ = {};
    return EditController::initialize(context);
}

Steinberg::tresult PLUGIN_API Controller::notify(Steinberg::Vst::IMessage* message)
{
    if (dataExchange_.onMessage(message))
        return Steinberg::kResultTrue;
    return EditController::notify(message);
}

void PLUGIN_API Controller::queueOpened(
    Steinberg::Vst::DataExchangeUserContextID userContextID,
    Steinberg::uint32 blockSize,
    Steinberg::TBool& dispatchOnBackgroundThread)
{
    if (userContextID == kAnalysisExchangeContext && blockSize >= sizeof(AnalysisExchangePacket))
        dispatchOnBackgroundThread = false;
}

void PLUGIN_API Controller::queueClosed(
    Steinberg::Vst::DataExchangeUserContextID userContextID)
{
    if (userContextID == kAnalysisExchangeContext)
        hasPacket_ = false;
}

void PLUGIN_API Controller::onDataExchangeBlocksReceived(
    Steinberg::Vst::DataExchangeUserContextID userContextID,
    Steinberg::uint32 numBlocks,
    Steinberg::Vst::DataExchangeBlock* blocks,
    Steinberg::TBool)
{
    if (userContextID != kAnalysisExchangeContext || !blocks)
        return;

    for (Steinberg::uint32 i = 0; i < numBlocks; ++i)
    {
        if (!blocks[i].data || blocks[i].size < sizeof(AnalysisExchangePacket))
            continue;

        AnalysisExchangePacket packet;
        std::memcpy(&packet, blocks[i].data, sizeof(packet));
        if (!hasPacket_ || packet.sequence >= latestPacket_.sequence)
        {
            latestPacket_ = packet;
            hasPacket_ = true;
        }
    }
}

Analysis::Assessment Controller::evaluateLatest(
    Analysis::AnalysisMode mode,
    Analysis::Genre genre,
    Analysis::Era era) const noexcept
{
    if (!hasPacket_)
    {
        Analysis::Metrics unavailable;
        unavailable.loudnessAvailable = false;
        unavailable.plrAvailable = false;
        unavailable.lraAvailable = false;
        unavailable.provisional = true;
        return Analysis::AssessmentModel::evaluate(unavailable, mode, genre, era);
    }

    return Analysis::AssessmentModel::evaluate(latestPacket_.metrics, mode, genre, era);
}
}
