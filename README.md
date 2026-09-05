# Analysator

Analysator is a VST3 plugin for genre-aware mix and master analysis and professional quality assessment.

## Core concept

- Analyze a mix or master without altering the audio signal
- User-selectable style/genre profile
- Immediate overall assessment in the compact view
- Optional detailed technical analysis
- German / English UI
- Clean, commercial studio-style interface

## Analysis areas

- Loudness
- True Peak
- Dynamics / Crest Factor / PLR / LRA
- Tonal Balance
- Low End
- Stereo Image
- Phase / Correlation
- L/R Balance

## Design principles

- Professional, restrained visual language
- No faces, thumbs, comic or novelty look
- Studio-inspired background artwork used subtly
- A dedicated status instrument provides the immediate overall verdict
- Main view stays simple; detailed diagnostics are optional
- Scoring is genre-aware and weighted rather than based on one fixed target

## Product identity

**ANALYSATOR**  
*Mix & Master Assessment*

The plugin analyzes, measures and evaluates. It does not process or alter the programme audio.

## Architecture

- `src/analysis/` — assessment model and scoring
- `src/dsp/` — analysis engine and metrology
- `src/plugin/` — VST3 processor/controller and data exchange
- `src/resource/` — VSTGUI resources
- `.github/workflows/` — Windows VST3 build pipeline
