#include "AnalysisEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Mixorator::DSP
{
namespace
{
constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kInvSqrt2 = 0.70710678118654752440084436210485;
constexpr double kLoudnessOffset = -0.691;
constexpr double kAbsoluteGateLufs = -70.0;
constexpr double kRelativeGateLu = 10.0;
constexpr double kMinimumEnergy = 1.0e-20;
constexpr double kTruePeakFir[12][4] = {
    { 0.0017089843750, -0.0291748046875, -0.0189208984375, -0.0083007812500 },
    { 0.0109863281250,  0.0292968750000,  0.0330810546875,  0.0148925781250 },
    {-0.0196533203125, -0.0517578125000, -0.0582275390625, -0.0266113281250 },
    { 0.0332031250000,  0.0891113281250,  0.1015625000000,  0.0476074218750 },
    {-0.0594482421875, -0.1665039062500, -0.2003173828125, -0.1022949218750 },
    { 0.1373291015625,  0.4650878906250,  0.7797851562500,  0.9721679687500 },
    { 0.9721679687500,  0.7797851562500,  0.4650878906250,  0.1373291015625 },
    {-0.1022949218750, -0.2003173828125, -0.1665039062500, -0.0594482421875 },
    { 0.0476074218750,  0.1015625000000,  0.0891113281250,  0.0332031250000 },
    {-0.0266113281250, -0.0582275390625, -0.0517578125000, -0.0196533203125 },
    { 0.0148925781250,  0.0330810546875,  0.0292968750000,  0.0109863281250 },
    {-0.0083007812500, -0.0189208984375, -0.0291748046875,  0.0017089843750 }
};
}

double AnalysisEngine::Biquad::process(double x) noexcept { const double y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y; }
void AnalysisEngine::Biquad::clear() noexcept { z1=0.0; z2=0.0; }

AnalysisEngine::Biquad AnalysisEngine::makeKWeightingShelf(double sr) noexcept
{
    constexpr double gainDb=3.999843853973347, f0=1681.974450955533, q=0.7071752369554196, vbExp=0.4996667741545416;
    const double k=std::tan(kPi*f0/sr), vh=std::pow(10.0,gainDb/20.0), vb=std::pow(vh,vbExp), a0=1.0+k/q+k*k;
    Biquad f; f.b0=(vh+vb*k/q+k*k)/a0; f.b1=2.0*(k*k-vh)/a0; f.b2=(vh-vb*k/q+k*k)/a0; f.a1=2.0*(k*k-1.0)/a0; f.a2=(1.0-k/q+k*k)/a0; return f;
}

AnalysisEngine::Biquad AnalysisEngine::makeKWeightingHighPass(double sr) noexcept
{
    constexpr double f0=38.13547087602444, q=0.5003270373238773;
    const double k=std::tan(kPi*f0/sr), a0=1.0+k/q+k*k;
    Biquad f; f.b0=1.0; f.b1=-2.0; f.b2=1.0; f.a1=2.0*(k*k-1.0)/a0; f.a2=(1.0-k/q+k*k)/a0; return f;
}

double AnalysisEngine::energyToLufs(double e) noexcept { return e<=kMinimumEnergy ? -std::numeric_limits<double>::infinity() : kLoudnessOffset+10.0*std::log10(e); }
double AnalysisEngine::linearToDbfs(double v) noexcept { return v>0.0 ? 20.0*std::log10(v) : -std::numeric_limits<double>::infinity(); }

void AnalysisEngine::prepare(double sr)
{
    sampleRate_=sr>1.0?sr:48000.0;
    momentarySamples_=std::max<std::size_t>(1,static_cast<std::size_t>(std::llround(sampleRate_*0.400)));
    shortTermSamples_=std::max<std::size_t>(1,static_cast<std::size_t>(std::llround(sampleRate_*3.000)));
    hopSamples_=std::max<std::size_t>(1,static_cast<std::size_t>(std::llround(sampleRate_*0.100)));
    momentaryRing_.assign(momentarySamples_,0.0); shortTermRing_.assign(shortTermSamples_,0.0);
    loudnessBlocks_.assign(4u*60u*60u*10u,0.0);
    shelf_[0]=makeKWeightingShelf(sampleRate_); shelf_[1]=shelf_[0]; highPass_[0]=makeKWeightingHighPass(sampleRate_); highPass_[1]=highPass_[0]; reset();
}

void AnalysisEngine::reset()
{
    std::fill(momentaryRing_.begin(),momentaryRing_.end(),0.0); std::fill(shortTermRing_.begin(),shortTermRing_.end(),0.0);
    momentaryWrite_=shortTermWrite_=momentaryValid_=shortTermValid_=samplesSinceBlock_=loudnessBlockCount_=0; momentarySum_=shortTermSum_=0.0;
    samplePeakLinear_=truePeakLinear_=0.0; rmsSumSquares_=0.0L; rmsSampleCount_=0;
    leftSumSquares_=rightSumSquares_=lrCrossSum_=midSumSquares_=sideSumSquares_=0.0L; stereoSampleCount_=0;
    dcSum_[0]=dcSum_[1]=0.0L; dcSampleCount_[0]=dcSampleCount_[1]=0; clippedSampleCountRaw_=nonFiniteSampleCountRaw_=0;
    for(auto& f:shelf_) f.clear(); for(auto& f:highPass_) f.clear(); for(auto& h:truePeakHistory_) std::fill(h,h+12,0.0);
    samplePeakDbfs_.store(-1000.0); truePeakDbtp_.store(-1000.0); rmsDbfs_.store(-1000.0); crestFactorDb_.store(0.0);
    momentaryLufs_.store(-1000.0); shortTermLufs_.store(-1000.0); lrBalanceDb_.store(0.0); correlation_.store(1.0);
    stereoWidthDb_.store(-1000.0); monoCompatibilityDb_.store(0.0); dcOffsetLeftDbfs_.store(-1000.0); dcOffsetRightDbfs_.store(-1000.0);
    clippedSampleCount_.store(0); nonFiniteSampleCount_.store(0);
}

void AnalysisEngine::pushWindowSample(std::vector<double>& r,std::size_t& w,std::size_t& valid,double& sum,double v) noexcept
{
    if(r.empty()) return; if(valid==r.size()) sum-=r[w]; else ++valid; r[w]=v; sum+=v; w=(w+1)%r.size();
}

void AnalysisEngine::processTruePeakSample(int ch,double s) noexcept
{
    auto& h=truePeakHistory_[ch]; for(int i=11;i>0;--i) h[i]=h[i-1]; h[0]=s; truePeakLinear_=std::max(truePeakLinear_,std::abs(s));
    for(int p=0;p<4;++p){ double y=0.0; for(int t=0;t<12;++t) y+=h[t]*kTruePeakFir[t][p]; truePeakLinear_=std::max(truePeakLinear_,std::abs(y)); }
}

void AnalysisEngine::updateStereoMetrics() noexcept
{
    if(!stereoSampleCount_) return; const long double eps=1.0e-30L, l=leftSumSquares_, r=rightSumSquares_, den=std::sqrt(std::max(l*r,eps));
    double corr=den>0.0L?static_cast<double>(lrCrossSum_/den):1.0; corr=std::max(-1.0,std::min(1.0,corr));
    double bal=0.0; if(l>eps&&r>eps) bal=10.0*std::log10(static_cast<double>(l/r)); else if(l>eps) bal=1000.0; else if(r>eps) bal=-1000.0;
    double width=-1000.0; if(midSumSquares_>eps&&sideSumSquares_>eps) width=10.0*std::log10(static_cast<double>(sideSumSquares_/midSumSquares_)); else if(sideSumSquares_>eps) width=1000.0;
    const long double se=l+r; double mono=0.0; if(se>eps&&midSumSquares_>eps) mono=10.0*std::log10(static_cast<double>(midSumSquares_/se)); else if(se>eps) mono=-1000.0;
    lrBalanceDb_.store(bal); correlation_.store(corr); stereoWidthDb_.store(width); monoCompatibilityDb_.store(mono);
}

void AnalysisEngine::updateTechnicalMetrics() noexcept
{
    for(int ch=0;ch<2;++ch){ double db=-1000.0; if(dcSampleCount_[ch]) db=linearToDbfs(std::abs(static_cast<double>(dcSum_[ch]/static_cast<long double>(dcSampleCount_[ch])))); (ch?dcOffsetRightDbfs_:dcOffsetLeftDbfs_).store(db); }
    clippedSampleCount_.store(clippedSampleCountRaw_); nonFiniteSampleCount_.store(nonFiniteSampleCountRaw_);
}

template <typename Sample>
void AnalysisEngine::processBlock(Sample* const* channels,int numChannels,int numSamples) noexcept
{
    if(!channels||numChannels<=0||numSamples<=0||momentaryRing_.empty()||shortTermRing_.empty()) return; const int nCh=std::min(numChannels,2);
    for(int i=0;i<numSamples;++i){ double weightedEnergy=0.0;
        for(int ch=0;ch<nCh;++ch){ if(!channels[ch]) continue; const double s=static_cast<double>(channels[ch][i]);
            if(!std::isfinite(s)){ ++nonFiniteSampleCountRaw_; continue; }
            if(std::abs(s)>1.0) ++clippedSampleCountRaw_; dcSum_[ch]+=static_cast<long double>(s); ++dcSampleCount_[ch];
            samplePeakLinear_=std::max(samplePeakLinear_,std::abs(s)); processTruePeakSample(ch,s); rmsSumSquares_+=static_cast<long double>(s)*s; ++rmsSampleCount_;
            double f=shelf_[ch].process(s); f=highPass_[ch].process(f); weightedEnergy+=f*f;
        }
        if(nCh==2&&channels[0]&&channels[1]){ const double ld=static_cast<double>(channels[0][i]), rd=static_cast<double>(channels[1][i]); if(std::isfinite(ld)&&std::isfinite(rd)){ const long double l=ld,r=rd,m=(l+r)*kInvSqrt2,s=(l-r)*kInvSqrt2; leftSumSquares_+=l*l; rightSumSquares_+=r*r; lrCrossSum_+=l*r; midSumSquares_+=m*m; sideSumSquares_+=s*s; ++stereoSampleCount_; } }
        pushWindowSample(momentaryRing_,momentaryWrite_,momentaryValid_,momentarySum_,weightedEnergy); pushWindowSample(shortTermRing_,shortTermWrite_,shortTermValid_,shortTermSum_,weightedEnergy);
        if(++samplesSinceBlock_>=hopSamples_){ samplesSinceBlock_=0; if(momentaryValid_==momentarySamples_){ const double ms=momentarySum_/momentarySamples_; momentaryLufs_.store(energyToLufs(ms)); if(loudnessBlockCount_<loudnessBlocks_.size()) loudnessBlocks_[loudnessBlockCount_++]=ms; } if(shortTermValid_==shortTermSamples_) shortTermLufs_.store(energyToLufs(shortTermSum_/shortTermSamples_)); }
    }
    const double peak=linearToDbfs(samplePeakLinear_), tp=linearToDbfs(truePeakLinear_); double rms=-std::numeric_limits<double>::infinity(), crest=0.0;
    if(rmsSampleCount_){ const double rl=std::sqrt(static_cast<double>(rmsSumSquares_/rmsSampleCount_)); if(rl>0.0){ rms=linearToDbfs(rl); if(samplePeakLinear_>0.0) crest=peak-rms; } }
    samplePeakDbfs_.store(peak); truePeakDbtp_.store(tp); rmsDbfs_.store(rms); crestFactorDb_.store(crest); updateStereoMetrics(); updateTechnicalMetrics();
}

void AnalysisEngine::process(float* const* c,int n,int s) noexcept { processBlock(c,n,s); }
void AnalysisEngine::process(double* const* c,int n,int s) noexcept { processBlock(c,n,s); }

double AnalysisEngine::calculateIntegratedLufs() const noexcept
{
    if(!loudnessBlockCount_) return -std::numeric_limits<double>::infinity(); const double ag=std::pow(10.0,(kAbsoluteGateLufs-kLoudnessOffset)/10.0); double sum=0.0; std::size_t count=0;
    for(std::size_t i=0;i<loudnessBlockCount_;++i) if(loudnessBlocks_[i]>=ag){sum+=loudnessBlocks_[i];++count;} if(!count) return -std::numeric_limits<double>::infinity();
    const double rg=std::pow(10.0,(energyToLufs(sum/count)-kRelativeGateLu-kLoudnessOffset)/10.0), gate=std::max(ag,rg); sum=0.0; count=0;
    for(std::size_t i=0;i<loudnessBlockCount_;++i) if(loudnessBlocks_[i]>=gate){sum+=loudnessBlocks_[i];++count;} return count?energyToLufs(sum/count):-std::numeric_limits<double>::infinity();
}

double AnalysisEngine::calculatePlrDb() const noexcept
{
    const double i=calculateIntegratedLufs(),tp=truePeakDbtp_.load(); return (!std::isfinite(i)||!std::isfinite(tp))?std::numeric_limits<double>::quiet_NaN():tp-i;
}
}
