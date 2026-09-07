#include "AssessmentModel.h"

#include <algorithm>
#include <cmath>

namespace Mixorator::Analysis
{
namespace
{
struct Profile
{
    double loudnessMin, loudnessMax, plrMin, plrMax, lraMin, lraMax;
    double loudnessMargin, plrMargin, lraMargin;
    double loudnessWeight, plrWeight, lraWeight;
};

struct TonalProfile
{
    std::array<double, 4> min;
    std::array<double, 4> max;
    std::array<double, 4> margin;
};

Profile profileFor(AnalysisMode mode, Genre genre, Era era) noexcept
{
    const bool vintage = era == Era::Vintage;
    if (mode == AnalysisMode::Mix)
    {
        switch (genre)
        {
            case Genre::Rock:               return {-27,-14, 9,22, 3,18, 7,7,9, .35,.40,.25};
            case Genre::Metal:              return {-25,-13, 8,19, 2,14, 7,6,8, .35,.45,.20};
            case Genre::Pop:                return {-25,-13, 8,19, 2,14, 7,6,8, .35,.45,.20};
            case Genre::Techno:
            case Genre::HouseEdm:           return {-23,-12, 7,17, 1.5,12, 6,6,7, .35,.45,.20};
            case Genre::DrumAndBass:        return {-24,-12, 7,18, 1.5,13, 7,6,8, .35,.45,.20};
            case Genre::HipHopTrap:         return {-24,-12, 7,18, 1.5,13, 7,6,8, .35,.45,.20};
            case Genre::RnBSoul:            return {-28,-14, 9,23, 3,18, 8,8,9, .30,.40,.30};
            case Genre::Electronic:         return {-28,-12, 8,23, 2,19, 8,8,10,.30,.40,.30};
            case Genre::Ambient:            return {-33,-14,11,29, 4,25,10,10,12,.20,.40,.40};
            case Genre::AcousticFolk:       return {-30,-15,11,26, 4,22, 8,8,10,.25,.40,.35};
            case Genre::Jazz:               return {-30,-15,11,27, 4,23, 8,8,10,.25,.40,.35};
            case Genre::Classical:          return {-34,-16,14,32, 6,28, 9,10,12,.20,.40,.40};
            case Genre::Cinematic:          return {-31,-14,11,29, 4,25, 9,9,11,.25,.40,.35};
            case Genre::General:            return {-28,-13, 9,23, 3,19, 8,8,10,.30,.45,.25};
        }
    }

    switch (genre)
    {
        case Genre::Rock: return vintage ? Profile{-20,-9,10,24,3,19,6,7,9,.35,.40,.25} : Profile{-15,-7,7,18,2,14,5,6,8,.40,.40,.20};
        case Genre::Metal:return vintage ? Profile{-18,-8,9,21,2,16,6,7,8,.40,.40,.20} : Profile{-13,-5.5,5,14,1,10,5,5,7,.45,.40,.15};
        case Genre::Pop:  return vintage ? Profile{-19,-9,9,21,2,16,6,7,8,.40,.40,.20} : Profile{-15,-6.5,6,15,1,11,5,5,7,.45,.40,.15};
        case Genre::Techno:
        case Genre::HouseEdm:return vintage ? Profile{-18,-8.5,8,19,2,14,6,6,8,.45,.40,.15} : Profile{-13,-5.5,4.5,13,1,9,5,5,6,.45,.40,.15};
        case Genre::DrumAndBass:return vintage ? Profile{-19,-8,8,20,2,15,6,7,8,.45,.40,.15} : Profile{-14,-5,4.5,13,1,9,5,5,6,.45,.40,.15};
        case Genre::HipHopTrap:return vintage ? Profile{-19,-9,9,21,2,15,6,7,8,.40,.40,.20} : Profile{-14,-5.5,5,14,1,10,5,5,7,.45,.40,.15};
        case Genre::RnBSoul:return vintage ? Profile{-22,-10,10,25,3,20,7,8,10,.30,.40,.30} : Profile{-18,-7.5,7,19,2,15,6,7,9,.35,.40,.25};
        case Genre::Electronic:return vintage ? Profile{-23,-9.5,10,26,4,22,7,8,10,.30,.40,.30} : Profile{-18,-6.5,6,19,2,16,6,7,9,.35,.40,.25};
        case Genre::Ambient:return vintage ? Profile{-27,-11,13,31,5,27,9,10,12,.15,.40,.45} : Profile{-24,-9,10,27,4,23,8,9,11,.20,.40,.40};
        case Genre::AcousticFolk:return vintage ? Profile{-24,-12,12,28,5,23,7,8,10,.25,.40,.35} : Profile{-21,-9.5,9,23,3,20,7,7,9,.25,.40,.35};
        case Genre::Jazz:return vintage ? Profile{-24,-12,13,29,5,24,7,8,10,.25,.40,.35} : Profile{-22,-10,10,25,4,21,7,8,9,.25,.40,.35};
        case Genre::Classical:return {-27,-13,15,34,7,30,9,11,13,.15,.40,.45};
        case Genre::Cinematic:return vintage ? Profile{-26,-11,13,31,5,27,9,10,12,.20,.40,.40} : Profile{-22,-8,9,25,3,21,8,8,10,.25,.40,.35};
        case Genre::General:return vintage ? Profile{-22,-10,11,26,4,21,7,8,10,.30,.45,.25} : Profile{-18,-7.5,7,20,2,16,6,7,9,.35,.45,.20};
    }
    return {-18,-7.5,7,20,2,16,6,7,9,.35,.45,.20};
}

TonalProfile tonalProfileFor(Genre genre) noexcept
{
    switch (genre)
    {
        case Genre::Rock:               return {{{18,25,15,1}}, {{45,50,40,15}}, {{15,15,15,8}}};
        case Genre::Metal:              return {{{15,20,20,2}}, {{40,45,45,18}}, {{15,15,15,8}}};
        case Genre::Pop:                return {{{20,25,18,2}}, {{45,50,40,15}}, {{15,15,15,8}}};
        case Genre::Techno:
        case Genre::HouseEdm:           return {{{30,18,12,1}}, {{60,40,35,12}}, {{18,15,15,7}}};
        case Genre::DrumAndBass:        return {{{32,16,12,1}}, {{65,40,35,12}}, {{18,15,15,7}}};
        case Genre::HipHopTrap:         return {{{30,18,10,1}}, {{65,40,30,10}}, {{18,15,15,7}}};
        case Genre::RnBSoul:            return {{{20,25,12,1}}, {{50,55,35,12}}, {{15,17,15,7}}};
        case Genre::Electronic:         return {{{22,18,12,1}}, {{58,48,38,16}}, {{18,17,17,8}}};
        case Genre::Ambient:            return {{{12,22,10,0.5}}, {{48,58,38,15}}, {{17,18,18,9}}};
        case Genre::AcousticFolk:       return {{{10,30,20,2}}, {{35,60,45,18}}, {{12,18,15,9}}};
        case Genre::Jazz:               return {{{10,28,20,2}}, {{38,58,45,18}}, {{13,18,15,9}}};
        case Genre::Classical:          return {{{10,25,20,4}}, {{35,50,45,20}}, {{13,16,15,10}}};
        case Genre::Cinematic:          return {{{20,20,15,2}}, {{55,48,42,18}}, {{18,16,16,9}}};
        case Genre::General:            return {{{12,18,10,0.5}}, {{55,60,50,22}}, {{18,20,20,11}}};
    }
    return {{{12,18,10,0.5}}, {{55,60,50,22}}, {{18,20,20,11}}};
}

double rangeScore(double v,double lo,double hi,double margin) noexcept
{
    if (!std::isfinite(v)) return 0.0;
    if (v >= lo && v <= hi) return 100.0;
    const double d = v < lo ? lo-v : v-hi;
    return std::clamp(100.0 - d/std::max(margin,0.001)*50.0,0.0,100.0);
}

bool tonalDataAvailable(const Metrics& m) noexcept
{
    const double total=m.tonalPercent[0]+m.tonalPercent[1]+m.tonalPercent[2]+m.tonalPercent[3];
    return std::isfinite(total) && total>99.0 && total<101.0;
}

double tonalPlausibilityScore(const Metrics& m, Genre genre) noexcept
{
    const auto p=tonalProfileFor(genre);
    constexpr double weights[4]={.35,.30,.25,.10};
    double score=0.0;
    for (int i=0;i<4;++i)
        score += weights[i]*rangeScore(m.tonalPercent[static_cast<std::size_t>(i)],p.min[i],p.max[i],p.margin[i]);
    return std::clamp(score,0.0,100.0);
}

bool tonalExtremeOutlier(const Metrics& m, Genre genre) noexcept
{
    const auto p=tonalProfileFor(genre);
    for (std::size_t i=0;i<4;++i)
    {
        const double v=m.tonalPercent[i];
        const double margin=std::max(p.margin[i],0.001);
        if (!std::isfinite(v))
            return false;
        if (v < p.min[i]-2.0*margin || v > p.max[i]+2.0*margin)
            return true;
    }
    return false;
}

double localStereoPenalty(const Metrics& m) noexcept
{
    double severity=0.0;
    if (std::isfinite(m.worstLocalCorrelation) && m.worstLocalCorrelation<0.0)
        severity += std::min(8.0,-m.worstLocalCorrelation*8.0);
    if (std::isfinite(m.worstLocalMonoCompatibilityDb) && m.worstLocalMonoCompatibilityDb<-3.0)
        severity += std::min(8.0,(-3.0-m.worstLocalMonoCompatibilityDb)*1.5);

    const double prevalence=std::clamp(m.negativeCorrelationPercent,0.0,100.0);
    const double prevalencePenalty = prevalence<=1.0 ? 0.0 : std::min(8.0,(prevalence-1.0)*0.35);
    return std::min(16.0,severity+prevalencePenalty);
}

// Extremely loud masters with simultaneously low PLR are a technical
// trade-off even when they are stylistically correct. Keep this deliberately
// conservative: density alone can move EXCELLENT to GOOD, but can never make
// an otherwise clean master ATTENTION or CRITICAL.
double masterDensityPenalty(const Metrics& m, AnalysisMode mode) noexcept
{
    if (mode != AnalysisMode::Master || !m.plrAvailable ||
        !std::isfinite(m.integratedLufs) || !std::isfinite(m.plrDb))
        return 0.0;

    const double loudnessStress=std::max(0.0,m.integratedLufs+8.0);
    const double plrStress=std::max(0.0,8.0-m.plrDb);
    const double jointStress=std::min(loudnessStress,plrStress);
    return std::min(15.0,6.0*jointStress);
}

// Streaming delivery is a service-independent robustness estimate. It does
// not treat a specific loudness-normalization target as a quality criterion.
// Instead, it assesses how much true-peak headroom remains for lossy codec
// reconstruction and transcoding. Clean masters below -1 dBTP receive no
// penalty; masters close to 0 dBTP are conservatively rated GOOD, while
// positive true peaks can enter ATTENTION.
double streamingHeadroomPenalty(double truePeakDbtp) noexcept
{
    if (!std::isfinite(truePeakDbtp) || truePeakDbtp < -1.0)
        return 0.0;
    if (truePeakDbtp <= 0.0)
        return 11.0 + 14.0*(truePeakDbtp + 1.0);
    return std::min(50.0,25.0 + 15.0*truePeakDbtp);
}

double clippingPenalty(std::uint64_t clippedSamples, double cap) noexcept
{
    if (clippedSamples == 0)
        return 0.0;
    return std::min(cap, 15.0 + 5.0*std::log10(1.0 + static_cast<double>(clippedSamples)));
}

Verdict verdictFor(double s) noexcept
{
    if (s>=90) return Verdict::Excellent;
    if (s>=75) return Verdict::Good;
    if (s>=50) return Verdict::Attention;
    return Verdict::Critical;
}
}

Assessment AssessmentModel::evaluate(const Metrics& m, AnalysisMode mode, Genre genre, Era era) noexcept
{
    Assessment a;
    a.provisional = m.provisional;

    if (!std::isfinite(m.truePeakDbtp) || !m.loudnessAvailable || !std::isfinite(m.integratedLufs))
    {
        a.technicalVerdict=a.styleVerdict=a.pcmDeliveryVerdict=a.streamingDeliveryVerdict=a.overallVerdict=Verdict::InsufficientData;
        a.technicalScore=a.styleScore=a.pcmDeliveryScore=a.streamingDeliveryScore=a.overallScore=0.0;
        return a;
    }

    double technical=100.0;
    if (m.nonFiniteSamples>0) technical=0.0;
    technical-=clippingPenalty(m.clippedSamples,30.0);
    if (m.truePeakDbtp>0.0) technical-=std::min(40.0,20.0+m.truePeakDbtp*10.0);
    if (m.correlation<0.0) technical-=std::min(30.0,-m.correlation*30.0);
    if (m.monoCompatibilityDb<-3.0) technical-=std::min(30.0,(-3.0-m.monoCompatibilityDb)*5.0);
    technical-=localStereoPenalty(m);
    if (std::abs(m.lrBalanceDb)>3.0) technical-=std::min(15.0,(std::abs(m.lrBalanceDb)-3.0)*3.0);
    if (m.dcOffsetLeftDbfs>-50.0 || m.dcOffsetRightDbfs>-50.0) technical-=10.0;
    technical-=masterDensityPenalty(m,mode);
    technical=std::clamp(technical,0.0,100.0);

    const Profile p=profileFor(mode,genre,era);
    double weightedStyle=0.0;
    double usedWeight=0.0;
    if (m.loudnessAvailable && std::isfinite(m.integratedLufs))
    {
        weightedStyle += p.loudnessWeight*rangeScore(m.integratedLufs,p.loudnessMin,p.loudnessMax,p.loudnessMargin);
        usedWeight += p.loudnessWeight;
    }
    if (m.plrAvailable && std::isfinite(m.plrDb))
    {
        weightedStyle += p.plrWeight*rangeScore(m.plrDb,p.plrMin,p.plrMax,p.plrMargin);
        usedWeight += p.plrWeight;
    }
    if (m.lraAvailable && std::isfinite(m.lraLu))
    {
        weightedStyle += p.lraWeight*rangeScore(m.lraLu,p.lraMin,p.lraMax,p.lraMargin);
        usedWeight += p.lraWeight;
    }
    double style = usedWeight>0.0 ? weightedStyle/usedWeight : 0.0;

    if (tonalDataAvailable(m))
    {
        const double tonal=tonalPlausibilityScore(m,genre);
        // Tonal balance remains secondary for ordinary creative variation.
        // A truly extreme single-band excursion, however, must prevent an
        // EXCELLENT style verdict even if loudness and dynamics are ideal.
        style = 0.85*style + 0.15*tonal;
        if (tonal < 35.0 || tonalExtremeOutlier(m,genre))
            style = std::min(style,89.0);
    }
    style=std::clamp(style,0.0,100.0);

    double pcm=0.0;
    double streaming=0.0;
    if (mode == AnalysisMode::Master)
    {
        pcm=100.0;
        if (m.nonFiniteSamples>0) pcm=0.0;
        pcm-=clippingPenalty(m.clippedSamples,35.0);
        if (m.truePeakDbtp>0.0) pcm-=std::min(50.0,20.0+m.truePeakDbtp*15.0);
        pcm=std::clamp(pcm,0.0,100.0);

        streaming=100.0;
        if (m.nonFiniteSamples>0)
        {
            streaming=0.0;
        }
        else
        {
            // Sample/full-scale clipping and positive true peak describe the
            // same ceiling-risk family. For streaming/transcoding robustness,
            // use the stronger of the two penalties instead of double-counting
            // the same event and turning small decoded overs into CRITICAL.
            const double ceilingPenalty=std::max(
                clippingPenalty(m.clippedSamples,35.0),
                streamingHeadroomPenalty(m.truePeakDbtp));
            streaming-=ceilingPenalty;
        }
        streaming=std::clamp(streaming,0.0,100.0);

        // Retained as an informational normalization preview only. It is not
        // used by the streaming-quality verdict.
        a.streamingGainDb=-14.0-m.integratedLufs;
    }

    double overall=0.60*technical+0.40*style;
    if (technical<50.0) overall=std::min(overall,49.0);
    else if (technical<75.0) overall=std::min(overall,74.0);

    a.technicalScore=technical;
    a.styleScore=style;
    a.pcmDeliveryScore=pcm;
    a.streamingDeliveryScore=streaming;
    a.overallScore=std::clamp(overall,0.0,100.0);
    a.technicalVerdict=verdictFor(a.technicalScore);
    a.styleVerdict=(a.styleScore<50.0 && a.technicalScore>=75.0)
        ? Verdict::Unusual
        : verdictFor(a.styleScore);
    a.overallVerdict=verdictFor(a.overallScore);
    if (mode == AnalysisMode::Master)
    {
        a.pcmDeliveryVerdict=verdictFor(a.pcmDeliveryScore);
        a.streamingDeliveryVerdict=verdictFor(a.streamingDeliveryScore);
    }
    else
    {
        a.pcmDeliveryVerdict=Verdict::InsufficientData;
        a.streamingDeliveryVerdict=Verdict::InsufficientData;
    }
    return a;
}
}
