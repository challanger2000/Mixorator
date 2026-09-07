#include "analysis/AssessmentModel.h"
#include <cmath>
#include <iostream>
namespace {
int fail(const char* m){std::cerr<<"FAIL: "<<m<<'\n';return 1;}
bool approx(double a,double b,double t=1e-12){return std::abs(a-b)<=t;}
Mixorator::Analysis::Metrics baseMetrics(){using namespace Mixorator::Analysis;Metrics m;m.integratedLufs=-10;m.truePeakDbtp=-1.2;m.plrDb=10;m.lraLu=5;m.correlation=.8;m.monoCompatibilityDb=-.5;m.worstLocalCorrelation=.8;m.worstLocalMonoCompatibilityDb=-.5;m.dcOffsetLeftDbfs=-90;m.dcOffsetRightDbfs=-90;return m;}
Mixorator::Analysis::Metrics tonal(std::array<double,8> v){auto m=baseMetrics();m.detailedTonalPercent=v;return m;}
bool finiteRatios(const Mixorator::Analysis::TonalRatioFeatures& f){return std::isfinite(f.subVsBassDb)&&std::isfinite(f.lowMidVsMidDb)&&std::isfinite(f.presenceVsMidDb)&&std::isfinite(f.upperPresenceVsPresenceDb)&&std::isfinite(f.brillianceVsUpperPresenceDb)&&std::isfinite(f.airVsBrillianceDb)&&std::isfinite(f.lowVsMidDb)&&std::isfinite(f.highVsMidDb);}
}
int main(){using namespace Mixorator::Analysis;
 Metrics empty=baseMetrics(); if(AssessmentModel::detailedTonalDataAvailable(empty))return fail("Empty detailed tonal data was treated as valid"); if(AssessmentModel::tonalRatioScore(empty,Genre::General)!=0)return fail("Missing data produced ratio score");
 Metrics equal=tonal({{12.5,12.5,12.5,12.5,12.5,12.5,12.5,12.5}});auto ef=AssessmentModel::tonalRatioFeatures(equal);if(!finiteRatios(ef)||!approx(ef.subVsBassDb,0)||!approx(ef.lowMidVsMidDb,0)||!approx(ef.highVsMidDb,10*std::log10(2.0)))return fail("Ratio extraction sanity check failed");
 const Metrics glow=tonal({{41.4372,36.8340,9.2006,9.9428,1.3891,.6383,.4488,.1093}}),ricky=tonal({{.6874,25.6551,21.2383,41.9215,9.4837,.7246,.2507,.0386}}),winnetou=tonal({{.4809,15.2061,32.3142,46.0280,5.7695,.1649,.0310,.0053}}),tchaikovsky=tonal({{2.2699,18.2910,27.6430,42.7908,8.7523,.1840,.0396,.0293}}),israel=tonal({{.0342,20.5488,70.7207,7.2947,.7576,.3366,.2462,.0611}});
 for(const Metrics* r:{&glow,&ricky,&winnetou,&tchaikovsky,&israel})if(!AssessmentModel::detailedTonalDataAvailable(*r)||!finiteRatios(AssessmentModel::tonalRatioFeatures(*r)))return fail("Measured reference produced invalid ratio data");
 auto gr=AssessmentModel::tonalRatioFeatures(glow),ir=AssessmentModel::tonalRatioFeatures(israel);if(gr.lowVsMidDb<=4)return fail("EDM low tilt not exposed");if(ir.lowMidVsMidDb<=8)return fail("Acoustic low-mid body not exposed");
 // Each measured anchor must be comfortably plausible in its intended genre.
 if(AssessmentModel::tonalRatioScore(glow,Genre::HouseEdm)<85)return fail("EDM ratio anchor scored too low");
 if(AssessmentModel::tonalRatioScore(ricky,Genre::Pop)<85)return fail("Pop ratio anchor scored too low");
 if(AssessmentModel::tonalRatioScore(winnetou,Genre::Cinematic)<85)return fail("Cinematic ratio anchor scored too low");
 if(AssessmentModel::tonalRatioScore(tchaikovsky,Genre::Classical)<85)return fail("Classical ratio anchor scored too low");
 if(AssessmentModel::tonalRatioScore(israel,Genre::AcousticFolk)<85)return fail("Acoustic ratio anchor scored too low");
 // Strongly characteristic references should prefer their matching family.
 if(AssessmentModel::tonalRatioScore(glow,Genre::HouseEdm)<=AssessmentModel::tonalRatioScore(glow,Genre::AcousticFolk))return fail("EDM ratio anchor did not prefer EDM");
 if(AssessmentModel::tonalRatioScore(israel,Genre::AcousticFolk)<=AssessmentModel::tonalRatioScore(israel,Genre::HouseEdm))return fail("Acoustic ratio anchor did not prefer Acoustic/Folk");
 // Genres without measured anchors deliberately inherit General rather than fake precision.
 if(!approx(AssessmentModel::tonalRatioScore(ricky,Genre::Metal),AssessmentModel::tonalRatioScore(ricky,Genre::General)))return fail("Uncalibrated genre did not fall back to General");
 // Production isolation remains absolute.
 Metrics plain=baseMetrics();plain.tonalPercent={{35,35,25,5}};Metrics detailed=plain;detailed.detailedTonalPercent={{20,24,8,20,12,7,5,4}};auto before=AssessmentModel::evaluate(plain,AnalysisMode::Master,Genre::HouseEdm,Era::Modern),after=AssessmentModel::evaluate(detailed,AnalysisMode::Master,Genre::HouseEdm,Era::Modern);
 if(!approx(before.technicalScore,after.technicalScore)||!approx(before.styleScore,after.styleScore)||!approx(before.pcmDeliveryScore,after.pcmDeliveryScore)||!approx(before.streamingDeliveryScore,after.streamingDeliveryScore)||!approx(before.overallScore,after.overallScore)||before.technicalVerdict!=after.technicalVerdict||before.styleVerdict!=after.styleVerdict||before.pcmDeliveryVerdict!=after.pcmDeliveryVerdict||before.streamingDeliveryVerdict!=after.streamingDeliveryVerdict||before.overallVerdict!=after.overallVerdict)return fail("Ratio calibration leaked into production verdicts");
 std::cout<<"All detailed tonal calibration tests passed.\n";return 0;}
