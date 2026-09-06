# Mixorator calibration sources and limits

## Primary empirical reference

Mixorator's assessment concept is informed in part by the RoEx / Queen Mary University of London study:

Angeliki Mourgela, Elio Quinton, Spyridon Bissas, Joshua D. Reiss, David Ronan,
"Exploring trends in audio mixes and masters: Insights from a dataset analysis",
AES 157th Convention, New York, 2024, Convention Paper 10186.

Public preprint: arXiv:2412.03373
Raw dataset: Zenodo DOI 10.5281/zenodo.13683186

The paper analyses 218,109 MixCheck Studio entries across 30 user-selected genres. It reports 67,838 entries identified by users as mixes and 150,217 as masters.

The study explicitly analyses:
- Integrated loudness using ITU-R BS.1770
- True peak using ITU-R BS.1770
- Clipping
- Mono compatibility
- Phase issues
- Stereo-field balance
- Dynamic-range compression
- Tonal profile in four bands: 20-250 Hz, 250-2000 Hz, 2000-8000 Hz, 8000-20000 Hz

Important study findings relevant to Mixorator include:
- Mixes are more dispersed in loudness and peak around approximately -23 LUFS; masters cluster more strongly around approximately -14 LUFS.
- More than half of mixes are reported as louder than -17.5 LUFS; 10.24% are below -23 LUFS.
- 79% of masters are louder than -14 LUFS and 91.55% are louder than -16 LUFS.
- Mixes show more under-compression; 46.43% were categorised as under-compressed.
- 51.63% of masters were categorised as optimally compressed.
- Mono-compatibility issues were reported for about 16.9% of mixes and 12.0% of masters.
- Phase issues were reported for about 16.3% of mixes and 15.6% of masters.
- Electronic, drum'n'bass and techno show stronger bass emphasis; acoustic, folk and blues lean more toward mid/high energy; orchestral and metal can show stronger high-frequency energy.

## What the RoEx study directly supports in Mixorator

The study strongly supports the overall architecture of separating:
- MIX versus MASTER assessment
- universal technical safety from genre-dependent style assessment
- loudness, dynamics, stereo/mono/phase and tonal analysis
- four broad tonal frequency regions
- genre-aware interpretation rather than one universal target

It also supports treating aesthetic/style metrics as trends rather than hard pass/fail standards.

## What is NOT directly supplied by the RoEx paper

The exact numeric profile ranges currently used by `AssessmentModel.cpp` are not published in the paper as authoritative per-genre target tables.

In particular, the following Mixorator values are calibration choices / engineering heuristics rather than values directly quoted from the RoEx publication:
- exact LUFS min/max ranges for each Mixorator genre
- exact PLR min/max ranges for each genre
- exact LRA min/max ranges for each genre
- scoring margins and weights
- exact tonal-percent min/max ranges
- the Modern versus Vintage split
- the final score thresholds Excellent / Good / Attention / Critical
- the 60/40 technical-versus-style overall weighting

The RoEx study's compression metric is also not identical to Mixorator's current PLR/LRA model. RoEx describes dynamic range using 20*log10(maximum / mean amplitude), compared with empirical genre-specific averages. Mixorator currently uses PLR and LRA as musically meaningful dynamics descriptors. Therefore the RoEx findings validate the need for genre-aware dynamics assessment, but do not directly validate Mixorator's exact PLR/LRA numeric ranges.

Likewise, the paper describes tonal analysis using the same four broad frequency bands, but its published results are categorical / distributional rather than a table of exact target energy percentages. Mixorator's exact tonal percentage ranges are therefore derived calibration values, not direct RoEx thresholds.

## Era handling

The RoEx dataset/paper does not define a Modern versus Vintage calibration. Mixorator's `Era::Modern` / `Era::Vintage` distinction is an independent product feature and must be treated as a separate heuristic calibration layer.

## Data limitations inherited from the reference study

The study itself notes that genre and MIX/MASTER status were selected by users, so misclassification can affect distributions. It also states that compression, tonal characteristics and stereo-field interpretations reflect empirical practice and current industry trends, not immutable standards.

For Mixorator this means:
- technical faults such as non-finite samples, clipping, positive true peak, severe phase/mono problems can be judged relatively strictly;
- genre/style deviations should remain advisory rather than being presented as objective defects;
- a clean stylistic outlier should be allowed to produce an `Unusual` style verdict rather than being labelled technically bad.

## Current implementation audit

`src/analysis/AssessmentModel.cpp` currently follows the above separation reasonably well:
- technical scoring is independent of Genre/Era;
- MIX and MASTER use different style profiles;
- MASTER alone receives PCM and streaming delivery verdicts;
- genre-dependent style is based on loudness, PLR, LRA and optional tonal plausibility;
- tonal plausibility contributes only 15% to style score;
- severe technical faults cap the overall result;
- technically clean stylistic outliers can become `Unusual`.

The current design is therefore conceptually consistent with the RoEx findings, but the exact per-genre calibration constants should be described as Mixorator calibration rather than as numbers directly taken from the RoEx dataset.

## Next calibration step

Before changing production scoring, use the public Zenodo dataset where practical to calculate distributions that can be mapped onto Mixorator's supported genres. Because the raw RoEx compression descriptor differs from PLR/LRA and the tonal results are categorical, any translation to Mixorator metrics must be explicitly documented as a derived mapping rather than a direct import.

No scoring constants should be changed merely to make Mixorator imitate the most common values in the dataset. The dataset contains many problematic mixes/masters and describes what users submitted, not a corpus of verified reference masters. It is best used to establish realistic distributions, prevalence and genre trends, while technical recommendations and delivery safety remain grounded in metering standards and engineering practice.
