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
        {"MIX","MIX"},{"MASTER","MASTER"},{"ANALYSE","ANALYZE"},{"ABSCHLIESSEN","FINALIZE"},{"DETAILS","DETAILS"},{"ZURÜCK","BACK"},{"RESET","RESET"},
        {"GENRE","GENRE"},{"ÄRA","ERA"},{"STATUS","STATE"},{"MESSWERTE","MEASUREMENTS"},{"BEWERTUNG","ASSESSMENT"},
        {"TECHNISCHE QUALITÄT","TECHNICAL QUALITY"},{"STIL-TREFFER","STYLE MATCH"},{"PCM / WAV","PCM / WAV"},{"STREAMING","STREAMING"},{"ANALYSESTATUS","ANALYSIS STATE"},
        {"INTEGRIERT","INTEGRATED"},{"TRUE PEAK","TRUE PEAK"},{"PLR","PLR"},{"LRA","LRA"},{"KORRELATION","CORRELATION"},{"MONO","MONO"},
        {"BEREIT","READY"},{"LIVE / VORLÄUFIG","LIVE / PROVISIONAL"},{"FINAL / WIRD ERSTELLT","FINAL / PENDING"},{"FINAL / ENDGÜLTIG","FINAL / DEFINITIVE"},
        {"MIX oder MASTER wählen, dann ANALYSE","Choose MIX or MASTER, then ANALYZE"},{"Kompletten Song von Anfang an abspielen","Play the complete song from the start"},
        {"Endergebnis angefordert","Final result requested"},{"Wiedergabe starten, falls Verarbeitung gestoppt ist","Start playback if processing is stopped"},
        {"Endgültiges Ergebnis wird erstellt","Preparing definitive result"},{"Warte auf Abschluss im Prozessor","Waiting for processor finalize"},
        {"Analyse abgeschlossen - endgültiges Ergebnis","Analysis complete - definitive result"},{"ANALYSE für eine neue Messung drücken","Press ANALYZE for a new measurement"},
        {"Am Ende FINAL / FINALIZE für das Ergebnis drücken","When finished, press FINAL / FINALIZE for result"},
        {"DETAILANALYSE","DETAILED ANALYSIS"},{"Technische Integrität, Stilkontext und Ausgabemessungen","Technical integrity, style context and delivery measurements"},
        {"HILFE","HELP"},{"SCHLIESSEN","CLOSE"},
        {"ABLAUF","WORKFLOW"},{"MIX oder MASTER wählen.\nANALYSE drücken und den kompletten Song von Anfang bis Ende abspielen.\nDanach im Main-Fenster FINALIZE drücken.","Choose MIX or MASTER.\nPress ANALYZE and play the complete song from beginning to end.\nThen press FINALIZE in the Main window."},
        {"MESSWERTE","MEASUREMENTS"},{"LUFS: integrierte Lautheit.  True Peak: Spitzenpegel zwischen Samples.\nPLR: Verhältnis von Peak zu Lautheit.  LRA: Lautheitsdynamik.\nKorrelation und Mono zeigen Stereo-/Monokompatibilität.","LUFS: integrated loudness.  True Peak: inter-sample peak level.\nPLR: peak-to-loudness ratio.  LRA: loudness dynamics.\nCorrelation and Mono indicate stereo/mono compatibility."},
        {"WICHTIG","IMPORTANT"},{"Analysator bewertet den kompletten Stereo-Mix bzw. das Master.\nEr verändert das Audiosignal nicht.","Analysator assesses the complete stereo mix or master.\nIt does not alter the audio signal."}
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
