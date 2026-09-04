#include "dsp/AnalysisEngine.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
constexpr double kPi=3.1415926535897932384626433832795;
std::vector<double> sine(double sr,double hz,double sec,double amp)
{
    std::vector<double> v(static_cast<std::size_t>(std::llround(sr*sec)));
    for(std::size_t i=0;i<v.size();++i)v[i]=amp*std::sin(2.0*kPi*hz*static_cast<double>(i)/sr);
    return v;
}
void process(Mixorator::DSP::AnalysisEngine& e,std::vector<double>& l,std::vector<double>& r)
{
    constexpr int bs=256;
    for(int p=0;p<static_cast<int>(l.size());p+=bs){const int n=std::min(bs,static_cast<int>(l.size())-p);double* c[2]={l.data()+p,r.data()+p};e.process(c,2,n);}
}
int fail(const char* text){std::cerr<<"FAIL: "<<text<<'\n';return 1;}
}

int main()
{
    constexpr double sr=48000.0;
    {
        Mixorator::DSP::AnalysisEngine e;e.prepare(sr);
        auto l=sine(sr,1000.0,2.0,0.5);auto r=l;process(e,l,r);
        if(e.worstLocalCorrelation()<0.999 || e.worstLocalMonoCompatibilityDb()<-0.01 || e.negativeCorrelationPercent()>0.01)
            return fail("Clean in-phase stereo produced a local phase fault");
    }
    {
        Mixorator::DSP::AnalysisEngine e;e.prepare(sr);
        auto l=sine(sr,1000.0,2.0,0.5);auto r=l;
        // Only 100 ms is polarity-inverted. The whole-program average remains
        // strongly positive, so this specifically proves local fault capture.
        const std::size_t begin=static_cast<std::size_t>(sr*0.9),end=static_cast<std::size_t>(sr*1.0);
        for(std::size_t i=begin;i<end;++i)r[i]=-r[i];
        process(e,l,r);
        if(e.correlation()<0.75)return fail("Test fixture no longer has a positive programme-wide correlation");
        if(e.worstLocalCorrelation()>-0.95)return fail("Short anti-phase section was averaged away");
        if(e.worstLocalMonoCompatibilityDb()>-100.0)return fail("Short mono cancellation was not captured");
        if(e.negativeCorrelationPercent()<4.0 || e.negativeCorrelationPercent()>6.0)return fail("Negative-window percentage is implausible");
    }
    std::cout<<"Local stereo fault tests passed.\n";
    return 0;
}
