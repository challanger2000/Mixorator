#include "AnalysisEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Mixorator::DSP
{
namespace
{
constexpr double kPi=3.1415926535897932384626433832795, kInvSqrt2=0.70710678118654752440084436210485;
constexpr double kLoudnessOffset=-0.691, kAbsoluteGateLufs=-70.0, kRelativeGateLu=10.0, kLraRelativeGateLu=20.0, kMinimumEnergy=1.0e-20;
constexpr double kTruePeakFir[12][4]={{0.0017089843750,-0.0291748046875,-0.0189208984375,-0.0083007812500},{0.0109863281250,0.0292968750000,0.0330810546875,0.0148925781250},{-0.0196533203125,-0.0517578125000,-0.0582275390625,-0.0266113281250},{0.0332031250000,0.0891113281250,0.1015625000000,0.0476074218750},{-0.0594482421875,-0.1665039062500,-0.2003173828125,-0.1022949218750},{0.1373291015625,0.4650878906250,0.7797851562500,0.9721679687500},{0.9721679687500,0.7797851562500,0.4650878906250,0.1373291015625},{-0.1022949218750,-0.2003173828125,-0.1665039062500,-0.0594482421875},{0.0476074218750,0.1015625000000,0.0891113281250,0.0332031250000},{-0.0266113281250,-0.0582275390625,-0.0517578125000,-0.0196533203125},{0.0148925781250,0.0330810546875,0.0292968750000,0.0109863281250},{-0.0083007812500,-0.0189208984375,-0.0291748046875,0.0017089843750}};
}

double AnalysisEngine::Biquad::process(double x) noexcept { const double y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y; }
void AnalysisEngine::Biquad::clear() noexcept { z1=z2=0.0; }
AnalysisEngine::Biquad AnalysisEngine::makeKWeightingShelf(double sr) noexcept { constexpr double g=3.999843853973347,f0=1681.974450955533,q=0.7071752369554196,vbe=0.4996667741545416; const double k=std::tan(kPi*f0/sr),vh=std::pow(10.0,g/20.0),vb=std::pow(vh,vbe),a0=1.0+k/q+k*k; Biquad f; f.b0=(vh+vb*k/q+k*k)/a0; f.b1=2.0*(k*k-vh)/a0; f.b2=(vh-vb*k/q+k*k)/a0; f.a1=2.0*(k*k-1.0)/a0; f.a2=(1.0-k/q+k*k)/a0; return f; }
AnalysisEngine::Biquad AnalysisEngine::makeKWeightingHighPass(double sr) noexcept { constexpr double f0=38.13547087602444,q=0.5003270373238773; const double k=std::tan(kPi*f0/sr),a0=1.0+k/q+k*k; Biquad f; f.b0=1.0; f.b1=-2.0; f.b2=1.0; f.a1=2.0*(k*k-1.0)/a0; f.a2=(1.0-k/q+k*k)/a0; return f; }
double AnalysisEngine::energyToLufs(double e) noexcept { return e<=kMinimumEnergy?-std::numeric_limits<double>::infinity():kLoudnessOffset+10.0*std::log10(e); }
double AnalysisEngine::linearToDbfs(double v) noexcept { return v>0.0?20.0*std::log10(v):-std::numeric_limits<double>::infinity(); }

void AnalysisEngine::prepare(double sr)
{
    sampleRate_=sr>1.0?sr:48000.0; momentarySamples_=std::max<std::size_t>(1,(std::size_t)std::llround(sampleRate_*0.4)); shortTermSamples_=std::max<std::size_t>(1,(std::size_t)std::llround(sampleRate_*3.0)); hopSamples_=std::max<std::size_t>(1,(std::size_t)std::llround(sampleRate_*0.1));
    momentaryRing_.assign(momentarySamples_,0.0); shortTermRing_.assign(shortTermSamples_,0.0); loudnessBlocks_.assign(4u*60u*60u*10u,0.0); lraShortTermBlocks_.assign(4u*60u*60u,0.0);
    shelf_[0]=makeKWeightingShelf(sampleRate_); shelf_[1]=shelf_[0]; highPass_[0]=makeKWeightingHighPass(sampleRate_); highPass_[1]=highPass_[0]; reset();
}
void AnalysisEngine::reset()
{
    std::fill(momentaryRing_.begin(),momentaryRing_.end(),0.0); std::fill(shortTermRing_.begin(),shortTermRing_.end(),0.0); momentaryWrite_=shortTermWrite_=momentaryValid_=shortTermValid_=samplesSinceBlock_=loudnessBlockCount_=lraShortTermBlockCount_=lraHopCounter_=0; momentarySum_=shortTermSum_=0.0;
    samplePeakLinear_=truePeakLinear_=0.0; rmsSumSquares_=0.0L; rmsSampleCount_=0; leftSumSquares_=rightSumSquares_=lrCrossSum_=midSumSquares_=sideSumSquares_=0.0L; stereoSampleCount_=0;
    localLeftSumSquares_=localRightSumSquares_=localCrossSum_=localMidSumSquares_=0.0L; localStereoSamples_=0; localStereoWindowCount_=negativeCorrelationWindowCount_=0; worstLocalCorrelationRaw_=1.0; worstLocalMonoCompatibilityDbRaw_=0.0;
    dcSum_[0]=dcSum_[1]=0.0L; dcSampleCount_[0]=dcSampleCount_[1]=0; clippedSampleCountRaw_=nonFiniteSampleCountRaw_=0;
    tonalWrite_=0; tonalFrameChannel_=0; std::fill(tonalInput_,tonalInput_+kFftSize,0.0); std::fill(fftReal_,fftReal_+kFftSize,0.0); std::fill(fftImag_,fftImag_+kFftSize,0.0); for(auto& e:tonalBandEnergy_) e=0.0L; for(auto& f:shelf_) f.clear(); for(auto& f:highPass_) f.clear(); for(auto& h:truePeakHistory_) std::fill(h,h+12,0.0);
    samplePeakDbfs_.store(-1000.0); truePeakDbtp_.store(-1000.0); rmsDbfs_.store(-1000.0); crestFactorDb_.store(0.0); momentaryLufs_.store(-1000.0); shortTermLufs_.store(-1000.0); lrBalanceDb_.store(0.0); correlation_.store(1.0); stereoWidthDb_.store(-1000.0); monoCompatibilityDb_.store(0.0); worstLocalCorrelation_.store(1.0); worstLocalMonoCompatibilityDb_.store(0.0); negativeCorrelationPercent_.store(0.0); dcOffsetLeftDbfs_.store(-1000.0); dcOffsetRightDbfs_.store(-1000.0); clippedSampleCount_.store(0); nonFiniteSampleCount_.store(0); lowBandPercent_.store(0.0); lowMidBandPercent_.store(0.0); highMidBandPercent_.store(0.0); highBandPercent_.store(0.0);
}
void AnalysisEngine::pushWindowSample(std::vector<double>& r,std::size_t& w,std::size_t& valid,double& sum,double v) noexcept { if(r.empty()) return; if(valid==r.size()) sum-=r[w]; else ++valid; r[w]=v; sum+=v; w=(w+1)%r.size(); }
void AnalysisEngine::processTruePeakSample(int ch,double s) noexcept { auto& h=truePeakHistory_[ch]; for(int i=11;i>0;--i) h[i]=h[i-1]; h[0]=s; truePeakLinear_=std::max(truePeakLinear_,std::abs(s)); for(int p=0;p<4;++p){double y=0;for(int t=0;t<12;++t)y+=h[t]*kTruePeakFir[t][p];truePeakLinear_=std::max(truePeakLinear_,std::abs(y));} }
void AnalysisEngine::updateStereoMetrics() noexcept { if(!stereoSampleCount_)return; const long double e=1e-30L,l=leftSumSquares_,r=rightSumSquares_,d=std::sqrt(std::max(l*r,e)); double c=d>0?double(lrCrossSum_/d):1.0;c=std::max(-1.0,std::min(1.0,c)); double b=0;if(l>e&&r>e)b=10*std::log10(double(l/r));else if(l>e)b=1000;else if(r>e)b=-1000; double w=-1000;if(midSumSquares_>e&&sideSumSquares_>e)w=10*std::log10(double(sideSumSquares_/midSumSquares_));else if(sideSumSquares_>e)w=1000; const long double se=l+r;double m=0;if(se>e&&midSumSquares_>e)m=10*std::log10(double(midSumSquares_/se));else if(se>e)m=-1000;lrBalanceDb_.store(b);correlation_.store(c);stereoWidthDb_.store(w);monoCompatibilityDb_.store(m); }
void AnalysisEngine::finishLocalStereoWindow() noexcept
{
    if(!localStereoSamples_)return;
    const long double e=1e-30L,l=localLeftSumSquares_,r=localRightSumSquares_,se=l+r;
    if(se>e)
    {
        const long double d=std::sqrt(std::max(l*r,e));
        double c=d>0?double(localCrossSum_/d):1.0;c=std::max(-1.0,std::min(1.0,c));
        double mono=localMidSumSquares_>e?10.0*std::log10(double(localMidSumSquares_/se)):-1000.0;
        worstLocalCorrelationRaw_=std::min(worstLocalCorrelationRaw_,c);
        worstLocalMonoCompatibilityDbRaw_=std::min(worstLocalMonoCompatibilityDbRaw_,mono);
        ++localStereoWindowCount_;if(c<0.0)++negativeCorrelationWindowCount_;
        worstLocalCorrelation_.store(worstLocalCorrelationRaw_);worstLocalMonoCompatibilityDb_.store(worstLocalMonoCompatibilityDbRaw_);
        negativeCorrelationPercent_.store(100.0*double(negativeCorrelationWindowCount_)/double(localStereoWindowCount_));
    }
    localLeftSumSquares_=localRightSumSquares_=localCrossSum_=localMidSumSquares_=0.0L;localStereoSamples_=0;
}
void AnalysisEngine::updateTechnicalMetrics() noexcept { for(int ch=0;ch<2;++ch){double d=-1000;if(dcSampleCount_[ch])d=linearToDbfs(std::abs(double(dcSum_[ch]/(long double)dcSampleCount_[ch])));(ch?dcOffsetRightDbfs_:dcOffsetLeftDbfs_).store(d);}clippedSampleCount_.store(clippedSampleCountRaw_);nonFiniteSampleCount_.store(nonFiniteSampleCountRaw_); }
void AnalysisEngine::pushTonalSample(double s,int n) noexcept { tonalInput_[tonalWrite_++]=std::isfinite(s)?s:0;if(tonalWrite_==kFftSize){analyseTonalFrame();tonalWrite_=0;tonalFrameChannel_=n>1?1-tonalFrameChannel_:0;} }
void AnalysisEngine::analyseTonalFrame() noexcept { for(std::size_t i=0;i<kFftSize;++i){double w=.5-.5*std::cos(2*kPi*i/double(kFftSize-1));fftReal_[i]=tonalInput_[i]*w;fftImag_[i]=0;}for(std::size_t i=1,j=0;i<kFftSize;++i){std::size_t bit=kFftSize>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j){std::swap(fftReal_[i],fftReal_[j]);std::swap(fftImag_[i],fftImag_[j]);}}for(std::size_t len=2;len<=kFftSize;len<<=1){double a=-2*kPi/len,wr0=std::cos(a),wi0=std::sin(a);for(std::size_t i=0;i<kFftSize;i+=len){double wr=1,wi=0;for(std::size_t j=0;j<len/2;++j){auto x=i+j,y=x+len/2;double vr=fftReal_[y]*wr-fftImag_[y]*wi,vi=fftReal_[y]*wi+fftImag_[y]*wr,ur=fftReal_[x],ui=fftImag_[x];fftReal_[x]=ur+vr;fftImag_[x]=ui+vi;fftReal_[y]=ur-vr;fftImag_[y]=ui-vi;double nw=wr*wr0-wi*wi0;wi=wr*wi0+wi*wr0;wr=nw;}}}double upper=std::min(20000.0,sampleRate_*.5);for(std::size_t k=1;k<=kFftSize/2;++k){double f=k*sampleRate_/kFftSize;if(f<20||f>upper)continue;long double e=(long double)fftReal_[k]*fftReal_[k]+(long double)fftImag_[k]*fftImag_[k];int b=f<250?0:(f<2000?1:(f<8000?2:3));tonalBandEnergy_[b]+=e;}updateTonalMetrics(); }
void AnalysisEngine::updateTonalMetrics() noexcept { long double t=tonalBandEnergy_[0]+tonalBandEnergy_[1]+tonalBandEnergy_[2]+tonalBandEnergy_[3];if(t<=1e-30L)return;lowBandPercent_.store(100*double(tonalBandEnergy_[0]/t));lowMidBandPercent_.store(100*double(tonalBandEnergy_[1]/t));highMidBandPercent_.store(100*double(tonalBandEnergy_[2]/t));highBandPercent_.store(100*double(tonalBandEnergy_[3]/t)); }

template<typename Sample> void AnalysisEngine::processBlock(Sample* const* c,int nc,int ns) noexcept
{
    if(!c||nc<=0||ns<=0||momentaryRing_.empty()||shortTermRing_.empty())return;int n=std::min(nc,2);
    for(int i=0;i<ns;++i){double we=0;for(int ch=0;ch<n;++ch){if(!c[ch])continue;double s=double(c[ch][i]);if(!std::isfinite(s)){++nonFiniteSampleCountRaw_;continue;}if(std::abs(s)>1)++clippedSampleCountRaw_;dcSum_[ch]+=s;++dcSampleCount_[ch];samplePeakLinear_=std::max(samplePeakLinear_,std::abs(s));processTruePeakSample(ch,s);rmsSumSquares_+=(long double)s*s;++rmsSampleCount_;double f=shelf_[ch].process(s);f=highPass_[ch].process(f);we+=f*f;}
        if(n==2&&c[0]&&c[1]){double ld=double(c[0][i]),rd=double(c[1][i]);if(std::isfinite(ld)&&std::isfinite(rd)){long double l=ld,r=rd,m=(l+r)*kInvSqrt2,s=(l-r)*kInvSqrt2;leftSumSquares_+=l*l;rightSumSquares_+=r*r;lrCrossSum_+=l*r;midSumSquares_+=m*m;sideSumSquares_+=s*s;++stereoSampleCount_;localLeftSumSquares_+=l*l;localRightSumSquares_+=r*r;localCrossSum_+=l*r;localMidSumSquares_+=m*m;if(++localStereoSamples_>=hopSamples_)finishLocalStereoWindow();}}
        int tc=n>1?tonalFrameChannel_:0;double ts=0;if(tc<n&&c[tc])ts=double(c[tc][i]);pushTonalSample(ts,n);pushWindowSample(momentaryRing_,momentaryWrite_,momentaryValid_,momentarySum_,we);pushWindowSample(shortTermRing_,shortTermWrite_,shortTermValid_,shortTermSum_,we);
        if(++samplesSinceBlock_>=hopSamples_){samplesSinceBlock_=0;if(momentaryValid_==momentarySamples_){double ms=momentarySum_/momentarySamples_;momentaryLufs_.store(energyToLufs(ms));if(loudnessBlockCount_<loudnessBlocks_.size())loudnessBlocks_[loudnessBlockCount_++]=ms;}if(shortTermValid_==shortTermSamples_){double st=energyToLufs(shortTermSum_/shortTermSamples_);shortTermLufs_.store(st);if(++lraHopCounter_>=10){lraHopCounter_=0;if(lraShortTermBlockCount_<lraShortTermBlocks_.size())lraShortTermBlocks_[lraShortTermBlockCount_++]=st;}}}}
    double p=linearToDbfs(samplePeakLinear_),tp=linearToDbfs(truePeakLinear_),r=-std::numeric_limits<double>::infinity(),cr=0;if(rmsSampleCount_){double rl=std::sqrt(double(rmsSumSquares_/rmsSampleCount_));if(rl>0){r=linearToDbfs(rl);if(samplePeakLinear_>0)cr=p-r;}}samplePeakDbfs_.store(p);truePeakDbtp_.store(tp);rmsDbfs_.store(r);crestFactorDb_.store(cr);updateStereoMetrics();updateTechnicalMetrics();
}
void AnalysisEngine::process(float* const* c,int n,int s) noexcept{processBlock(c,n,s);} void AnalysisEngine::process(double* const* c,int n,int s) noexcept{processBlock(c,n,s);}

double AnalysisEngine::calculateIntegratedLufs() const noexcept { if(!loudnessBlockCount_)return -std::numeric_limits<double>::infinity();double ag=std::pow(10.0,(kAbsoluteGateLufs-kLoudnessOffset)/10.0),sum=0;std::size_t count=0;for(std::size_t i=0;i<loudnessBlockCount_;++i)if(loudnessBlocks_[i]>=ag){sum+=loudnessBlocks_[i];++count;}if(!count)return -std::numeric_limits<double>::infinity();double rg=std::pow(10.0,(energyToLufs(sum/count)-kRelativeGateLu-kLoudnessOffset)/10.0),gate=std::max(ag,rg);sum=0;count=0;for(std::size_t i=0;i<loudnessBlockCount_;++i)if(loudnessBlocks_[i]>=gate){sum+=loudnessBlocks_[i];++count;}return count?energyToLufs(sum/count):-std::numeric_limits<double>::infinity(); }
double AnalysisEngine::calculatePlrDb() const noexcept { double i=calculateIntegratedLufs(),tp=truePeakDbtp_.load();return(!std::isfinite(i)||!std::isfinite(tp))?std::numeric_limits<double>::quiet_NaN():tp-i; }

double AnalysisEngine::calculateLoudnessRangeLu() const noexcept
{
    if(lraShortTermBlockCount_<2)return std::numeric_limits<double>::quiet_NaN();
    double sumEnergy=0.0;std::size_t absoluteCount=0;
    for(std::size_t i=0;i<lraShortTermBlockCount_;++i){double l=lraShortTermBlocks_[i];if(std::isfinite(l)&&l>=kAbsoluteGateLufs){sumEnergy+=std::pow(10.0,(l-kLoudnessOffset)/10.0);++absoluteCount;}}
    if(!absoluteCount)return std::numeric_limits<double>::quiet_NaN();
    const double relativeThreshold=energyToLufs(sumEnergy/absoluteCount)-kLraRelativeGateLu;
    constexpr double minL=-70.0,maxL=20.0,step=0.1; constexpr std::size_t bins=901; std::uint32_t histogram[bins]{};std::size_t gatedCount=0;
    const double gate=std::max(kAbsoluteGateLufs,relativeThreshold);
    for(std::size_t i=0;i<lraShortTermBlockCount_;++i){double l=lraShortTermBlocks_[i];if(!std::isfinite(l)||l<gate)continue;std::size_t b=(std::size_t)std::llround((std::max(minL,std::min(maxL,l))-minL)/step);if(b>=bins)b=bins-1;++histogram[b];++gatedCount;}
    if(gatedCount<2)return 0.0;
    auto percentile=[&](double q) noexcept { const std::size_t target=(std::size_t)std::ceil(q*gatedCount);std::size_t acc=0;for(std::size_t b=0;b<bins;++b){acc+=histogram[b];if(acc>=target)return minL+step*b;}return maxL;};
    return percentile(0.95)-percentile(0.10);
}
}
