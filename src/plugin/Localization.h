#pragma once

#include <cstddef>

namespace Mixorator::Localization
{
enum class Language { German = 0, English = 1 };

enum class Text
{
    Mix, Master, Analyze, Finalize, Details, Back, Reset,
    Genre, Era, State, Measurements, Assessment,
    TechnicalQuality, StyleMatch, PcmWav, Streaming, AnalysisState,
    Integrated, TruePeak, Plr, Lra, Correlation, Mono,
    Ready, LiveProvisional, FinalPending, FinalDefinitive,
    ChooseModeThenAnalyze, PlayCompleteSong,
    FinalResultRequested, StartPlaybackIfStopped,
    PreparingDefinitiveResult, WaitingForProcessorFinalize,
    AnalysisComplete, PressAnalyzeNewMeasurement,
    WhenFinishedFinalize,
    DetailedAnalysis, DetailedSubtitle,
    Help, Close,
    HelpWorkflowTitle, HelpWorkflowBody,
    HelpMetricsTitle, HelpMetricsBody,
    HelpSafetyTitle, HelpSafetyBody,
    Count
};

struct Pair { const char* de; const char* en; };

inline const Pair& pair(Text id) noexcept
{
    static constexpr Pair table[] = {
        {"MIX","MIX"},{"MASTER","MASTER"},{"ANALYSE","ANALYZE"},{"ABSCHLIESSEN","FINALIZE"},{"DETAILS","DETAILS"},{"ZURUECK","BACK"},{"RESET","RESET"},
        {"GENRE","GENRE"},{"AERA","ERA"},{"STATUS","STATE"},{"MESSWERTE","MEASUREMENTS"},{"BEWERTUNG","ASSESSMENT"},
        {"TECHNISCHE QUALITAET","TECHNICAL QUALITY"},{"STIL-TREFFER","STYLE MATCH"},{"PCM / WAV","PCM / WAV"},{"STREAMING","STREAMING"},{"ANALYSESTATUS","ANALYSIS STATE"},
        {"INTEGRIERT","INTEGRATED"},{"TRUE PEAK","TRUE PEAK"},{"PLR","PLR"},{"LRA","LRA"},{"KORRELATION","CORRELATION"},{"MONO","MONO"},
        {"BEREIT","READY"},{"LIVE / VORLAEUFIG","LIVE / PROVISIONAL"},{"FINAL / WIRD ERSTELLT","FINAL / PENDING"},{"FINAL / ENDGUELTIG","FINAL / DEFINITIVE"},
        {"MIX oder MASTER waehlen, dann ANALYSE","Choose MIX or MASTER, then ANALYZE"},{"Kompletten Song von Anfang an abspielen","Play the complete song from the start"},
        {"Endergebnis angefordert","Final result requested"},{"Wiedergabe starten, falls Verarbeitung gestoppt ist","Start playback if processing is stopped"},
        {"Endgueltiges Ergebnis wird erstellt","Preparing definitive result"},{"Warte auf Abschluss im Prozessor","Waiting for processor finalize"},
        {"Analyse abgeschlossen - endgueltiges Ergebnis","Analysis complete - definitive result"},{"ANALYSE fuer eine neue Messung druecken","Press ANALYZE for a new measurement"},
        {"Am Ende FINAL / FINALIZE fuer das Ergebnis druecken","When finished, press FINAL / FINALIZE for result"},
        {"DETAILANALYSE","DETAILED ANALYSIS"},{"Technische Integritaet, Stilkontext und Ausgabemessungen","Technical integrity, style context and delivery measurements"},
        {"HILFE","HELP"},{"SCHLIESSEN","CLOSE"},
        {"ABLAUF","WORKFLOW"},{"MIX oder MASTER waehlen. ANALYSE druecken und den kompletten Song von Anfang bis Ende abspielen. Danach FINALIZE im Main-Fenster bzw. FINAL in Details druecken.","Choose MIX or MASTER. Press ANALYZE and play the complete song from beginning to end. Then press FINALIZE in Main or FINAL in Details."},
        {"MESSWERTE","MEASUREMENTS"},{"LUFS: integrierte Lautheit. True Peak: Spitzenpegel zwischen Samples. PLR: Verhaeltnis von Peak zu Lautheit. LRA: Lautheitsdynamik. Korrelation und Mono zeigen Stereo-/Monokompatibilitaet.","LUFS: integrated loudness. True Peak: inter-sample peak level. PLR: peak-to-loudness ratio. LRA: loudness dynamics. Correlation and Mono indicate stereo/mono compatibility."},
        {"WICHTIG","IMPORTANT"},{"Analysator bewertet den kompletten Stereo-Mix bzw. das Master. Er veraendert das Audiosignal nicht.","Analysator assesses the complete stereo mix or master. It does not alter the audio signal."}
    };
    static_assert(sizeof(table)/sizeof(table[0]) == static_cast<std::size_t>(Text::Count));
    return table[static_cast<std::size_t>(id)];
}

inline const char* get(Text id, Language language) noexcept
{
    const auto& p = pair(id);
    return language == Language::German ? p.de : p.en;
}
}
