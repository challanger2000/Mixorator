#include "dsp/AnalysisEngine.h"
#include "dsp/AnalysisSnapshot.h"
#include "analysis/AssessmentInput.h"
#include "analysis/AssessmentModel.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
constexpr double kPi=3.1415926535897932384626433832795;
bool approx(double a,double b,double t){return std::abs(a-b)<=t;}
int fail(const char* m){std::cerr<<"FAIL: "<<m<<'\n';return 1;}
void processStereo(Mixorator::DSP::AnalysisEngine& e,const std::vector<double>& l,const std::vector<double>& r,int bs=256)
{
    for(int p=0;p<static_cast<int>(l.size());p+=bs){const int n=std::min(bs,static_cast<int>(l.size())-p);double* c[2]={const_cast<double*>(l.data()+p),const_cast<double*>(r.data()+p)};e.process(c,2,n);}
}
std::vector<double> sine(double sr,double hz,double sec,double amp,double ph=0){std::vector<double> v(static_cast<std::size_t>(std::llround(sr*sec)));for(std::size_t i=0;i<v.size();++i)v[i]=amp*std::sin(2*kPi*hz*static_cast<double>(i)/sr+ph);return v;}
Mixorator::Analysis::Metrics cleanMetrics(){Mixorator::Analysis::Metrics m;m.integratedLufs=-12;m.truePeakDbtp=-2.1;m.plrDb=10;m.lraLu=6;m.crestFactorDb=10;m.correlation=.7;m.monoCompatibilityDb=-.5;m.worstLocalCorrelation=.7;m.worstLocalMonoCompatibilityDb=-.5;m.negativeCorrelationPercent=0;m.lrBalanceDb=.2;m.dcOffsetLeftDbfs=-90;m.dcOffsetRightDbfs=-90;return m;}
}

int main()
{
using Mixorator::DSP::AnalysisEngine;using namespace Mixorator::Analysis;constexpr double sr=48000;
{
 AnalysisEngine e;e.prepare(sr);auto l=sine(sr,1000,4,.5);auto r=l;processStereo(e,l,r);
 if(e.truePeakDbtp()+1e-9<e.samplePeakDbfs())return fail("True Peak fell below Sample Peak");
 if(!approx(e.correlation(),1,1e-6)||!approx(e.lrBalanceDb(),0,1e-6)||!approx(e.monoCompatibilityDb(),0,1e-6))return fail("In-phase stereo metrics failed");
 if(std::abs(e.calculateLoudnessRangeLu())>.2)return fail("Constant programme LRA is not approximately 0 LU");
 const auto s=Mixorator::DSP::AnalysisSnapshot::capture(e);
 if(!s.valid||!approx(s.samplePeakDbfs,e.samplePeakDbfs(),1e-12)||!approx(s.truePeakDbtp,e.truePeakDbtp(),1e-12)||!approx(s.integratedLufs,e.calculateIntegratedLufs(),1e-12))return fail("Final snapshot did not preserve analyzer metrics");
 if(!std::isfinite(s.plrDb))return fail("Final snapshot produced invalid PLR");
 const double directLra=e.calculateLoudnessRangeLu();
 if(std::isfinite(directLra)){if(!std::isfinite(s.loudnessRangeLu)||!approx(s.loudnessRangeLu,directLra,1e-12))return fail("Final snapshot did not preserve LRA");}
 else if(std::isfinite(s.loudnessRangeLu))return fail("Final snapshot changed insufficient LRA data into a finite value");

 const auto liveMetrics=AssessmentInput::fromLive(e);
 if(!liveMetrics.provisional||!liveMetrics.loudnessAvailable||liveMetrics.plrAvailable||liveMetrics.lraAvailable)return fail("LIVE metric availability flags are wrong");
 if(!approx(liveMetrics.integratedLufs,e.shortTermLufs(),1e-12))return fail("LIVE loudness does not use Short-Term LUFS");
 const auto liveAssessment=AssessmentModel::evaluate(liveMetrics,AnalysisMode::Master,Genre::General,Era::Modern);
 if(!liveAssessment.provisional||liveAssessment.overallVerdict==Verdict::InsufficientData)return fail("LIVE assessment was not produced as provisional");

 const auto finalMetrics=AssessmentInput::fromFinal(s);
 if(finalMetrics.provisional||!finalMetrics.loudnessAvailable||!finalMetrics.plrAvailable)return fail("FINAL metric availability flags are wrong");
 if(!approx(finalMetrics.integratedLufs,s.integratedLufs,1e-12)||!approx(finalMetrics.plrDb,s.plrDb,1e-12))return fail("FINAL assessment mapping changed whole-program metrics");
 const auto finalAssessment=AssessmentModel::evaluate(finalMetrics,AnalysisMode::Master,Genre::General,Era::Modern);
 if(finalAssessment.provisional||finalAssessment.overallVerdict==Verdict::InsufficientData)return fail("FINAL assessment was not produced as definitive");
}
{
 AnalysisEngine e;e.prepare(sr);auto l=sine(sr,1000,1,.5);auto r=l;processStereo(e,l,r);
 const auto liveMetrics=AssessmentInput::fromLive(e);
 const auto a=AssessmentModel::evaluate(liveMetrics,AnalysisMode::Mix,Genre::Pop,Era::Modern);
 if(liveMetrics.loudnessAvailable||a.overallVerdict!=Verdict::InsufficientData)return fail("LIVE assessment did not wait for enough Short-Term data");

 const auto snapshot=Mixorator::DSP::AnalysisSnapshot::capture(e);
 if(!snapshot.valid)return fail("Short FINAL capture was not marked as a captured snapshot");
 const auto finalMetrics=AssessmentInput::fromFinal(snapshot);
 if(finalMetrics.loudnessAvailable||finalMetrics.plrAvailable||finalMetrics.lraAvailable)return fail("Short FINAL programme incorrectly exposed definitive whole-program metrics");
 const auto finalAssessment=AssessmentModel::evaluate(finalMetrics,AnalysisMode::Master,Genre::Pop,Era::Modern);
 if(finalAssessment.overallVerdict!=Verdict::InsufficientData)return fail("Short FINAL programme produced a definitive verdict");
}
{
 AnalysisEngine e;e.prepare(sr);auto l=sine(sr,1000,4,.5);auto r=l;for(auto&x:r)x=-x;processStereo(e,l,r);
 if(!approx(e.correlation(),-1,1e-6)||e.monoCompatibilityDb()>-100)return fail("Anti-phase detection failed");
}
{
 AnalysisEngine e;e.prepare(sr);auto l=sine(sr,12000,1,.95,kPi/4);auto r=l;processStereo(e,l,r);if(e.truePeakDbtp()+1e-9<e.samplePeakDbfs())return fail("True Peak stress invariant failed");
}
{
 AnalysisEngine e;e.prepare(sr);auto l=sine(sr,100,2,.5);auto r=l;processStereo(e,l,r);if(e.lowBandPercent()<90)return fail("100 Hz tonal classification failed");
}
{
 AnalysisEngine e;e.prepare(sr);auto l=sine(sr,4000,2,.5);auto r=l;processStereo(e,l,r);if(e.highMidBandPercent()<90)return fail("4 kHz tonal classification failed");
}
{
 Metrics m=cleanMetrics();m.integratedLufs=-8;m.truePeakDbtp=-2.1;m.plrDb=7;m.lraLu=4;
 const auto a=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Metal,Era::Modern);
 if(a.technicalScore<99||a.styleScore<85||a.pcmDeliveryScore<99||a.streamingDeliveryScore<99)return fail("Safe loud Metal master was incorrectly penalized");
 if(!approx(a.streamingGainDb,-6.0,1e-9))return fail("Streaming gain estimate is wrong");
}
{
 Metrics m=cleanMetrics();m.integratedLufs=-8;m.truePeakDbtp=-1;m.plrDb=7;m.lraLu=4;
 const auto a=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Metal,Era::Modern);
 if(a.styleScore<85||a.pcmDeliveryScore<99)return fail("Streaming concern contaminated master/PCM quality");
 if(a.streamingDeliveryScore>=90)return fail("Codec headroom risk did not lower streaming compatibility");
}
{
 Metrics m=cleanMetrics();m.integratedLufs=-8;m.plrDb=7;m.lraLu=4;
 const auto x=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Rock,Era::Modern);const auto y=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Rock,Era::Vintage);
 if(!approx(x.technicalScore,y.technicalScore,1e-9)||!approx(x.pcmDeliveryScore,y.pcmDeliveryScore,1e-9)||!approx(x.streamingDeliveryScore,y.streamingDeliveryScore,1e-9))return fail("Era altered universal safety/delivery");
 if(x.styleScore<=y.styleScore)return fail("Modern/vintage Rock profiles are not differentiated");
}
{
 Metrics d=cleanMetrics();d.integratedLufs=-18;d.plrDb=20;d.lraLu=15;Metrics c=d;c.plrDb=5;c.lraLu=1;
 if(AssessmentModel::evaluate(d,AnalysisMode::Master,Genre::Classical,Era::Modern).styleScore<=AssessmentModel::evaluate(c,AnalysisMode::Master,Genre::Classical,Era::Modern).styleScore+20)return fail("Classical dynamics profile insufficient");
}
{
 Metrics m=cleanMetrics();m.integratedLufs=-20;m.plrDb=14;m.lraLu=8;
 if(AssessmentModel::evaluate(m,AnalysisMode::Mix,Genre::Pop,Era::Modern).styleScore<=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Pop,Era::Modern).styleScore)return fail("MIX profile is not distinct from MASTER");
}
{
 Metrics m=cleanMetrics();m.integratedLufs=-10;m.plrDb=9;m.lraLu=5;m.tonalPercent={{60,20,15,5}};
 const auto techno=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Techno,Era::Modern);
 const auto acoustic=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::AcousticFolk,Era::Modern);
 if(techno.styleScore<=acoustic.styleScore+5.0)return fail("Genre-aware tonal balance did not distinguish bass-heavy Techno from Acoustic/Folk");
 if(!approx(techno.technicalScore,acoustic.technicalScore,1e-9))return fail("Tonal style scoring contaminated technical safety");
}
{
 Metrics clean=cleanMetrics();
 const auto base=AssessmentModel::evaluate(clean,AnalysisMode::Master,Genre::General,Era::Modern);
 Metrics shortFault=clean;shortFault.worstLocalCorrelation=-1.0;shortFault.worstLocalMonoCompatibilityDb=-20.0;shortFault.negativeCorrelationPercent=5.0;
 const auto shortResult=AssessmentModel::evaluate(shortFault,AnalysisMode::Master,Genre::General,Era::Modern);
 if(shortResult.technicalScore>=90.0||shortResult.technicalScore<80.0)return fail("Short severe stereo fault was not conservatively downgraded into warning/good range");
 if(!approx(shortResult.styleScore,base.styleScore,1e-9)||!approx(shortResult.pcmDeliveryScore,base.pcmDeliveryScore,1e-9)||!approx(shortResult.streamingDeliveryScore,base.streamingDeliveryScore,1e-9))return fail("Local stereo fault contaminated style or delivery scoring");
}
{
 Metrics isolated=cleanMetrics();isolated.worstLocalCorrelation=-0.2;isolated.worstLocalMonoCompatibilityDb=-4.0;isolated.negativeCorrelationPercent=.5;
 const auto a=AssessmentModel::evaluate(isolated,AnalysisMode::Master,Genre::General,Era::Modern);
 if(a.technicalScore<95.0)return fail("Tiny isolated stereo anomaly was over-penalized");
}
{
 Metrics m=cleanMetrics();m.integratedLufs=-9;m.truePeakDbtp=2;m.plrDb=8;m.lraLu=4;m.correlation=-1;m.monoCompatibilityDb=-1000;m.worstLocalCorrelation=-1;m.worstLocalMonoCompatibilityDb=-1000;m.negativeCorrelationPercent=100;m.dcOffsetLeftDbfs=-20;m.dcOffsetRightDbfs=-20;m.clippedSamples=100;
 const auto a=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Metal,Era::Modern);if(a.technicalScore>=50||a.overallScore>=50)return fail("Critical technical faults were averaged away");
}
{
 Metrics m=cleanMetrics();for(int mo=0;mo<2;++mo)for(int er=0;er<2;++er)for(int g=0;g<=static_cast<int>(Genre::General);++g){const auto a=AssessmentModel::evaluate(m,static_cast<AnalysisMode>(mo),static_cast<Genre>(g),static_cast<Era>(er));if(!std::isfinite(a.technicalScore)||!std::isfinite(a.styleScore)||!std::isfinite(a.pcmDeliveryScore)||!std::isfinite(a.streamingDeliveryScore)||!std::isfinite(a.overallScore)||!std::isfinite(a.streamingGainDb))return fail("A profile produced non-finite output");}
}
std::cout<<"All Mixorator deterministic tests passed.\n";return 0;
}