#include "MixoratorController.h"
#include "../analysis/AssessmentInput.h"
#include "../dsp/AnalysisSnapshot.h"

#include <cstring>

namespace Mixorator
{
Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context)
{
    hasPacket_ = false;
    latestPacket_ = {};
    requestedFinalGeneration_ = 0;
    finalSnapshotGeneration_ = 0;
    return EditController::initialize(context);
}

Steinberg::tresult PLUGIN_API Controller::notify(Steinberg::Vst::IMessage* message)
{
    if (consumeFinalSnapshotMessage(message))
        return Steinberg::kResultTrue;
    if (dataExchange_.onMessage(message))
        return Steinberg::kResultTrue;
    return EditController::notify(message);
}

Steinberg::tresult Controller::requestAnalysisState(Steinberg::int64 state) noexcept
{
    if (state != kAnalysisStateLive && state != kAnalysisStateFinal)
        return Steinberg::kInvalidArgument;

    auto* message = allocateMessage();
    if (!message)
        return Steinberg::kOutOfMemory;

    message->setMessageID(kSetAnalysisStateMessage);
    auto result = Steinberg::kResultFalse;
    if (auto* attributes = message->getAttributes())
    {
        if (attributes->setInt(kAnalysisStateKey, state) == Steinberg::kResultTrue)
            result = sendMessage(message);
    }
    message->release();
    return result;
}

Steinberg::tresult Controller::requestLiveAnalysis() noexcept
{
    return requestAnalysisState(kAnalysisStateLive);
}

Steinberg::tresult Controller::requestFinalAnalysis() noexcept
{
    return requestAnalysisState(kAnalysisStateFinal);
}

bool Controller::consumeFinalSnapshotMessage(Steinberg::Vst::IMessage* message) noexcept
{
    if (!message || !message->getMessageID() ||
        std::strcmp(message->getMessageID(), kFinalSnapshotMessage) != 0)
        return false;

    auto* attributes = message->getAttributes();
    if (!attributes)
        return true;

    Steinberg::int64 generationValue = 0;
    const void* data = nullptr;
    Steinberg::uint32 size = 0;
    if (attributes->getInt(kFinalSnapshotGenerationKey, generationValue) != Steinberg::kResultTrue ||
        attributes->getBinary(kFinalSnapshotDataKey, data, size) != Steinberg::kResultTrue ||
        generationValue < 0 || !data || size != sizeof(DSP::AnalysisSnapshot))
        return true;

    const auto generation = static_cast<std::uint64_t>(generationValue);
    if (!hasPacket_ || latestPacket_.finalState == 0 ||
        latestPacket_.finalizationGeneration != generation)
        return true;

    DSP::AnalysisSnapshot snapshot;
    std::memcpy(&snapshot, data, sizeof(snapshot));
    if (!snapshot.valid)
        return true;

    latestPacket_.metrics = Analysis::AssessmentInput::fromFinal(snapshot);
    finalSnapshotGeneration_ = generation;
    return true;
}

void Controller::requestFinalSnapshot(std::uint64_t generation) noexcept
{
    if (generation == 0 || generation == requestedFinalGeneration_)
        return;

    auto* message = allocateMessage();
    if (!message)
        return;

    message->setMessageID(kRequestFinalSnapshotMessage);
    if (auto* attributes = message->getAttributes())
    {
        attributes->setInt(kFinalSnapshotGenerationKey, static_cast<Steinberg::int64>(generation));
        if (sendMessage(message) == Steinberg::kResultTrue)
            requestedFinalGeneration_ = generation;
    }
    message->release();
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
    {
        hasPacket_ = false;
        requestedFinalGeneration_ = 0;
        finalSnapshotGeneration_ = 0;
    }
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
        if (hasPacket_ && packet.sequence < latestPacket_.sequence)
            continue;

        latestPacket_ = packet;
        hasPacket_ = true;

        if (packet.finalState != 0)
            requestFinalSnapshot(packet.finalizationGeneration);
        else
        {
            requestedFinalGeneration_ = 0;
            finalSnapshotGeneration_ = 0;
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

    if (latestPacket_.finalState != 0 && !hasDefinitiveFinalSnapshot())
    {
        auto waiting = latestPacket_.metrics;
        waiting.loudnessAvailable = false;
        waiting.plrAvailable = false;
        waiting.lraAvailable = false;
        waiting.provisional = true;
        return Analysis::AssessmentModel::evaluate(waiting, mode, genre, era);
    }

    return Analysis::AssessmentModel::evaluate(latestPacket_.metrics, mode, genre, era);
}
}
