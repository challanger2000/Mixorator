#include "dsp/AnalysisEngine.h"
#include "dsp/AnalysisSnapshot.h"
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
Mixorator::Analysis::Metrics cleanMetrics(){Mixorator::Analysis::Metrics m;m.integratedLufs=-12;m.truePeakDbtp=-2.1;m.plrDb=10;m.lraLu=6;m.crestFactorDb=10;m.correlation=.7;m.monoCompatibilityDb=-.5;m.lrBalanceDb=.2;m.dcOffsetLeftDbfs=-90;m.dcOffsetRightDbfs=-90;return m;}
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
// Loud modern Metal: strong musical master, safe PCM, streaming normalization is informational.
{
 Metrics m=cleanMetrics();m.integratedLufs=-8;m.truePeakDbtp=-2.1;m.plrDb=7;m.lraLu=4;
 const auto a=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Metal,Era::Modern);
 if(a.technicalScore<99||a.styleScore<85||a.pcmDeliveryScore<99||a.streamingDeliveryScore<99)return fail("Safe loud Metal master was incorrectly penalized");
 if(!approx(a.streamingGainDb,-6.0,1e-9))return fail("Streaming gain estimate is wrong");
}
// Same master with insufficient codec headroom: CD/PCM remains fine, streaming gets warning.
{
 Metrics m=cleanMetrics();m.integratedLufs=-8;m.truePeakDbtp=-1;m.plrDb=7;m.lraLu=4;
 const auto a=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Metal,Era::Modern);
 if(a.styleScore<85||a.pcmDeliveryScore<99)return fail("Streaming concern contaminated master/PCM quality");
 if(a.streamingDeliveryScore>=90)return fail("Codec headroom risk did not lower streaming compatibility");
}
// Era changes style, not technical or delivery safety.
{
 Metrics m=cleanMetrics();m.integratedLufs=-8;m.plrDb=7;m.lraLu=4;
 const auto x=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Rock,Era::Modern);const auto y=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Rock,Era::Vintage);
 if(!approx(x.technicalScore,y.technicalScore,1e-9)||!approx(x.pcmDeliveryScore,y.pcmDeliveryScore,1e-9)||!approx(x.streamingDeliveryScore,y.streamingDeliveryScore,1e-9))return fail("Era altered universal safety/delivery");
 if(x.styleScore<=y.styleScore)return fail("Modern/vintage Rock profiles are not differentiated");
}
// Classical rewards dynamics.
{
 Metrics d=cleanMetrics();d.integratedLufs=-18;d.plrDb=20;d.lraLu=15;Metrics c=d;c.plrDb=5;c.lraLu=1;
 if(AssessmentModel::evaluate(d,AnalysisMode::Master,Genre::Classical,Era::Modern).styleScore<=AssessmentModel::evaluate(c,AnalysisMode::Master,Genre::Classical,Era::Modern).styleScore+20)return fail("Classical dynamics profile insufficient");
}
// MIX and MASTER are distinct.
{
 Metrics m=cleanMetrics();m.integratedLufs=-20;m.plrDb=14;m.lraLu=8;
 if(AssessmentModel::evaluate(m,AnalysisMode::Mix,Genre::Pop,Era::Modern).styleScore<=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Pop,Era::Modern).styleScore)return fail("MIX profile is not distinct from MASTER");
}
// Critical technical faults cap overall.
{
 Metrics m=cleanMetrics();m.integratedLufs=-9;m.truePeakDbtp=2;m.plrDb=8;m.lraLu=4;m.correlation=-1;m.monoCompatibilityDb=-1000;m.dcOffsetLeftDbfs=-20;m.dcOffsetRightDbfs=-20;m.clippedSamples=100;
 const auto a=AssessmentModel::evaluate(m,AnalysisMode::Master,Genre::Metal,Era::Modern);if(a.technicalScore>=50||a.overallScore>=50)return fail("Critical technical faults were averaged away");
}
// Every profile produces finite outputs.
{
 Metrics m=cleanMetrics();for(int mo=0;mo<2;++mo)for(int er=0;er<2;++er)for(int g=0;g<=static_cast<int>(Genre::General);++g){const auto a=AssessmentModel::evaluate(m,static_cast<AnalysisMode>(mo),static_cast<Genre>(g),static_cast<Era>(er));if(!std::isfinite(a.technicalScore)||!std::isfinite(a.styleScore)||!std::isfinite(a.pcmDeliveryScore)||!std::isfinite(a.streamingDeliveryScore)||!std::isfinite(a.overallScore)||!std::isfinite(a.streamingGainDb))return fail("A profile produced non-finite output");}
}
std::cout<<"All Mixorator deterministic tests passed.\n";return 0;
}
