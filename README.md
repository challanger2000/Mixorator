# Mixorator

Mixorator is a VST3 plugin concept for genre-based mix analysis and professional quality assessment.

## Core concept

- Analyze a mix without altering the audio signal
- User-selectable style/genre profile
- Simple primary result using professional status faces
- Optional expandable technical details
- German / English UI
- Clean, commercial studio-style interface

## Planned analysis areas

- Loudness
- True Peak
- Dynamics / Crest Factor
- Tonal Balance
- Low End
- Stereo Image
- Phase / Correlation
- L/R Balance

## Design principles

- Professional, restrained visual language
- No comic or novelty look
- Studio-inspired background artwork used subtly
- Main view stays simple; detailed diagnostics are optional
- Scoring must be genre-aware and weighted rather than based on one fixed target

## Initial architecture

- `src/` — plugin source code
- `src/dsp/` — analysis modules
- `src/profiles/` — genre/style profiles and scoring rules
- `src/ui/` — GUI
- `src/i18n/` — German/English strings
- `.github/workflows/` — Windows VST3 build pipeline

Status: project scaffold initialized.
