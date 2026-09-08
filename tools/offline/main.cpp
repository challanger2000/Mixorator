#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "dsp/AnalysisEngine.h"
#include "dsp/AnalysisSnapshot.h"
#include "analysis/AssessmentModel.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Mixorator::DSP::AnalysisEngine;
using Mixorator::DSP::AnalysisSnapshot;
using Mixorator::Analysis::AssessmentModel;
using Mixorator::Analysis::Metrics;

namespace
{
bool supported(const fs::path& p)
{
    std::string e=p.extension().string();
    std::transform(e.begin(),e.end(),e.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return e==".wav" || e==".flac" || e==".mp3";
}

std::string csv(const std::string& value)
{
    std::string s=value; std::size_t pos=0;
    while((pos=s.find('"',pos))!=std::string::npos){s.insert(pos,"\"");pos+=2;}
    return '"'+s+'"';
}

bool digitallySilent(const std::vector<float>& interleaved, ma_uint64 frames)
{
    const std::size_t samples=static_cast<std::size_t>(frames)*2;
    for(std::size_t i=0;i<samples;++i) if(interleaved[i]!=0.0f) return false;
    return true;
}

Metrics metricsFrom(const AnalysisSnapshot& s)
{
    Metrics m;
    m.integratedLufs=s.integratedLufs; m.truePeakDbtp=s.truePeakDbtp; m.plrDb=s.plrDb; m.lraLu=s.loudnessRangeLu;
    m.crestFactorDb=s.crestFactorDb; m.correlation=s.correlation; m.monoCompatibilityDb=s.monoCompatibilityDb;
    m.worstLocalCorrelation=s.worstLocalCorrelation; m.worstLocalMonoCompatibilityDb=s.worstLocalMonoCompatibilityDb;
    m.negativeCorrelationPercent=s.negativeCorrelationPercent; m.lrBalanceDb=s.lrBalanceDb;
    m.dcOffsetLeftDbfs=s.dcOffsetLeftDbfs; m.dcOffsetRightDbfs=s.dcOffsetRightDbfs;
    m.clippedSamples=s.clippedSampleCount; m.nonFiniteSamples=s.nonFiniteSampleCount;
    m.tonalPercent=s.tonalPercent; m.detailedTonalPercent=s.detailedTonalPercent;
    m.loudnessAvailable=s.programmeContext; m.plrAvailable=s.programmeContext; m.lraAvailable=s.programmeContext;
    return m;
}

bool analyze(const fs::path& path, std::ofstream& out)
{
    // The plug-in accepts a stereo bus. Decode to stereo float PCM so the exact
    // same AnalysisEngine entry point and channel layout are used offline.
    ma_decoder_config config=ma_decoder_config_init(ma_format_f32,2,0);
    ma_decoder decoder{};
#ifdef _WIN32
    const ma_result initResult=ma_decoder_init_file_w(path.c_str(),&config,&decoder);
#else
    const ma_result initResult=ma_decoder_init_file(path.string().c_str(),&config,&decoder);
#endif
    if(initResult!=MA_SUCCESS){std::cerr<<"SKIP decode failed: "<<path.string()<<'\n';return false;}

    const double sampleRate=static_cast<double>(decoder.outputSampleRate);
    if(sampleRate<=1.0){ma_decoder_uninit(&decoder);std::cerr<<"SKIP invalid sample rate: "<<path.string()<<'\n';return false;}

    AnalysisEngine engine; engine.prepare(sampleRate); engine.reset();
    constexpr ma_uint64 framesPerBlock=4096;
    std::vector<float> interleaved(static_cast<std::size_t>(framesPerBlock)*2);
    std::vector<float> left(framesPerBlock),right(framesPerBlock);
    ma_uint64 totalFrames=0, analyzedFrames=0;

    for(;;)
    {
        ma_uint64 got=0;
        const ma_result r=ma_decoder_read_pcm_frames(&decoder,interleaved.data(),framesPerBlock,&got);
        if(got>0)
        {
            totalFrames+=got;
            // Match MixoratorProcessor: completely digital-silent input blocks are
            // not fed to AnalysisEngine. Non-silent blocks are passed unchanged.
            if(!digitallySilent(interleaved,got))
            {
                for(ma_uint64 i=0;i<got;++i){left[static_cast<std::size_t>(i)]=interleaved[2*i];right[static_cast<std::size_t>(i)]=interleaved[2*i+1];}
                float* channels[2]={left.data(),right.data()};
                engine.process(channels,2,static_cast<int>(got)); analyzedFrames+=got;
            }
        }
        if(r!=MA_SUCCESS || got==0) break;
    }
    ma_decoder_uninit(&decoder);

    const AnalysisSnapshot s=AnalysisSnapshot::capture(engine);
    const Metrics m=metricsFrom(s);
    const auto ratios=AssessmentModel::tonalRatioFeatures(m);
    const double seconds=static_cast<double>(totalFrames)/sampleRate;

    out<<csv(path.string())<<','<<std::fixed<<std::setprecision(0)<<sampleRate<<','<<totalFrames<<','<<analyzedFrames<<','<<std::setprecision(3)<<seconds
       <<','<<s.samplePeakDbfs<<','<<s.truePeakDbtp<<','<<s.rmsDbfs<<','<<s.crestFactorDb<<','<<s.integratedLufs<<','<<s.loudnessRangeLu<<','<<s.plrDb
       <<','<<s.lrBalanceDb<<','<<s.correlation<<','<<s.stereoWidthDb<<','<<s.monoCompatibilityDb<<','<<s.worstLocalCorrelation<<','<<s.worstLocalMonoCompatibilityDb<<','<<s.negativeCorrelationPercent
       <<','<<s.dcOffsetLeftDbfs<<','<<s.dcOffsetRightDbfs<<','<<s.clippedSampleCount<<','<<s.nonFiniteSampleCount;
    for(double v:s.detailedTonalPercent) out<<','<<v;
    out<<','<<ratios.subVsBassDb<<','<<ratios.lowMidVsMidDb<<','<<ratios.presenceVsMidDb<<','<<ratios.upperPresenceVsPresenceDb
       <<','<<ratios.brillianceVsUpperPresenceDb<<','<<ratios.airVsBrillianceDb<<','<<ratios.lowVsMidDb<<','<<ratios.highVsMidDb<<'\n';
    return true;
}
}

int main(int argc,char** argv)
{
    if(argc<2){std::cout<<"Mixorator Offline Analyzer\nUsage: MixoratorOfflineAnalyzer <file-or-folder> [results.csv]\nSupports WAV, FLAC and MP3. Audio stays local.\n";return 0;}
    const fs::path input=fs::u8path(argv[1]);
    const fs::path output=argc>=3?fs::u8path(argv[2]):fs::path("Mixorator-results.csv");
    if(!fs::exists(input)){std::cerr<<"Input not found.\n";return 2;}

    std::vector<fs::path> files;
    if(fs::is_regular_file(input)&&supported(input)) files.push_back(input);
    else if(fs::is_directory(input)) for(const auto& e:fs::recursive_directory_iterator(input)) if(e.is_regular_file()&&supported(e.path())) files.push_back(e.path());
    std::sort(files.begin(),files.end());
    if(files.empty()){std::cerr<<"No WAV, FLAC or MP3 files found.\n";return 3;}

    std::ofstream out(output,std::ios::binary);
    if(!out){std::cerr<<"Cannot create CSV.\n";return 4;}
    out<<"file,sample_rate,frames,analyzed_frames,duration_s,sample_peak_dbfs,true_peak_dbtp,rms_dbfs,crest_db,integrated_lufs,lra_lu,plr_db,lr_balance_db,correlation,stereo_width_db,mono_compat_db,worst_local_correlation,worst_local_mono_db,negative_corr_percent,dc_left_dbfs,dc_right_dbfs,clipped_samples,nonfinite_samples,sub_percent,bass_percent,lowmid_percent,mid_percent,presence_percent,upper_presence_percent,brilliance_percent,air_percent,sub_vs_bass_db,lowmid_vs_mid_db,presence_vs_mid_db,upper_presence_vs_presence_db,brilliance_vs_upper_presence_db,air_vs_brilliance_db,low_vs_mid_db,high_vs_mid_db\n";

    std::size_t ok=0;
    for(std::size_t i=0;i<files.size();++i){std::cout<<'['<<(i+1)<<'/'<<files.size()<<"] "<<files[i].filename().string()<<'\n';if(analyze(files[i],out))++ok;}
    std::cout<<"Done: "<<ok<<'/'<<files.size()<<" files -> "<<output.string()<<'\n';
    return ok==files.size()?0:5;
}
