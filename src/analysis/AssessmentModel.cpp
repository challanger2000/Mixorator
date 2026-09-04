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
            case Genre::HipHopTrap:         return {-24,-12, 7,18, 1.5,13, 7,6,8, .35,.45,.20};
            case Genre::ElectronicAmbient: return {-29,-13, 8,24, 2,20, 8,8,10,.30,.40,.30};
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
        case Genre::HipHopTrap:return vintage ? Profile{-19,-9,9,21,2,15,6,7,8,.40,.40,.20} : Profile{-14,-5.5,5,14,1,10,5,5,7,.45,.40,.15};
        case Genre::ElectronicAmbient:return vintage ? Profile{-23,-10,10,26,4,22,7,8,10,.30,.40,.30} : Profile{-20,-8,8,22,3,19,7,7,9,.30,.40,.30};
        case Genre::AcousticFolk:return vintage ? Profile{-24,-12,12,28,5,23,7,8,10,.25,.40,.35} : Profile{-21,-9.5,9,23,3,20,7,7,9,.25,.40,.35};
        case Genre::Jazz:return vintage ? Profile{-24,-12,13,29,5,24,7,8,10,.25,.40,.35} : Profile{-22,-10,10,25,4,21,7,8,9,.25,.40,.35};
        case Genre::Classical:return {-27,-13,15,34,7,30,9,11,13,.15,.40,.45};
        case Genre::Cinematic:return {-24,-9,11,29,4,25,8,9,11,.25,.40,.35};
        case Genre::General:return vintage ? Profile{-22,-10,11,26,4,21,7,8,10,.30,.45,.25} : Profile{-18,-7.5,7,20,2,16,6,7,9,.35,.45,.20};
    }
    return {-18,-7.5,7,20,2,16,6,7,9,.35,.45,.20};
}

double rangeScore(double v,double lo,double hi,double margin) noexcept
{
    if (!std::isfinite(v)) return 0.0;
    if (v >= lo && v <= hi) return 100.0;
    const double d = v < lo ? lo-v : v-hi;
    return std::clamp(100.0 - d/std::max(margin,0.001)*50.0,0.0,100.0);
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
    if (m.clippedSamples>0) technical-=std::min(30.0,15.0+5.0*std::log10(1.0+static_cast<double>(m.clippedSamples)));
    if (m.truePeakDbtp>0.0) technical-=std::min(40.0,20.0+m.truePeakDbtp*10.0);
    if (m.correlation<0.0) technical-=std::min(30.0,-m.correlation*30.0);
    if (m.monoCompatibilityDb<-3.0) technical-=std::min(30.0,(-3.0-m.monoCompatibilityDb)*5.0);
    if (std::abs(m.lrBalanceDb)>3.0) technical-=std::min(15.0,(std::abs(m.lrBalanceDb)-3.0)*3.0);
    if (m.dcOffsetLeftDbfs>-50.0 || m.dcOffsetRightDbfs>-50.0) technical-=10.0;
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
    const double tonalTotal=m.tonalPercent[0]+m.tonalPercent[1]+m.tonalPercent[2]+m.tonalPercent[3];
    if (tonalTotal>99.0 && tonalTotal<101.0 && (m.tonalPercent[0]>70.0 || m.tonalPercent[3]>50.0)) style=std::max(0.0,style-5.0);
    style=std::clamp(style,0.0,100.0);

    double pcm=100.0;
    if (m.nonFiniteSamples>0) pcm=0.0;
    if (m.clippedSamples>0) pcm-=std::min(35.0,15.0+5.0*std::log10(1.0+static_cast<double>(m.clippedSamples)));
    if (m.truePeakDbtp>0.0) pcm-=std::min(50.0,20.0+m.truePeakDbtp*15.0);
    pcm=std::clamp(pcm,0.0,100.0);

    const bool loud=m.integratedLufs>-14.0;
    const double recommendedTp=loud ? -2.0 : -1.0;
    double streaming=100.0;
    if (m.nonFiniteSamples>0) streaming=0.0;
    if (m.truePeakDbtp>recommendedTp)
        streaming-=std::min(50.0,10.0+(m.truePeakDbtp-recommendedTp)*20.0);
    streaming=std::clamp(streaming,0.0,100.0);
    a.streamingGainDb=-14.0-m.integratedLufs;

    double overall=0.60*technical+0.40*style;
    if (technical<50.0) overall=std::min(overall,49.0);
    else if (technical<75.0) overall=std::min(overall,74.0);

    a.technicalScore=technical;
    a.styleScore=style;
    a.pcmDeliveryScore=pcm;
    a.streamingDeliveryScore=streaming;
    a.overallScore=std::clamp(overall,0.0,100.0);
    a.technicalVerdict=verdictFor(a.technicalScore);
    a.styleVerdict=verdictFor(a.styleScore);
    a.pcmDeliveryVerdict=verdictFor(a.pcmDeliveryScore);
    a.streamingDeliveryVerdict=verdictFor(a.streamingDeliveryScore);
    a.overallVerdict=verdictFor(a.overallScore);
    return a;
}
}
