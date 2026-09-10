# Analysator calibration sources and limits

## Primary empirical reference

Analysator's assessment concept is informed in part by the RoEx / Queen Mary University of London study:

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

Important study findings relevant to Analysator include:
- Mixes are more dispersed in loudness and peak around approximately -23 LUFS; masters cluster more strongly around approximately -14 LUFS.
- More than half of mixes are reported as louder than -17.5 LUFS; 10.24% are below -23 LUFS.
- 79% of masters are louder than -14 LUFS and 91.55% are louder than -16 LUFS.
- Mixes show more under-compression; 46.43% were categorised as under-compressed.
- 51.63% of masters were categorised as optimally compressed.
- 68.58% of mixes but only 42.53% of masters were reported free from clipping.
- Mono-compatibility issues were reported for about 16.9% of mixes and 12.0% of masters.
- Phase issues were reported for about 16.3% of mixes and 15.6% of masters.
- Mix stereo fields were reported as wide in 17.94% and narrow in 39.04%; master stereo fields were wide in 39.36% and narrow in 16.45%.
- Electronic, drum'n'bass and techno show stronger bass emphasis; acoustic, folk and blues lean more toward mid/high energy; orchestral and metal can show stronger high-frequency energy.
- The paper ranks the most common MIX issues as: undercompression, stereo-field issues, too loud, clipping, too quiet, overcompression, mono incompatibility, phase issues.
- The paper ranks the most common MASTER issues as: too loud, clipping, overcompression, stereo-field issues, undercompression, phase issues, mono incompatibility, too quiet.

## What the RoEx study directly supports in Analysator

The study strongly supports the overall architecture of separating:
- MIX versus MASTER assessment
- universal technical safety from genre-dependent style assessment
- loudness, dynamics, stereo/mono/phase and tonal analysis
- four broad tonal frequency regions
- genre-aware interpretation rather than one universal target

It also supports treating aesthetic/style metrics as trends rather than hard pass/fail standards.

## What is NOT directly supplied by the RoEx paper

The exact numeric profile ranges currently used by `AssessmentModel.cpp` are not published in the paper as authoritative per-genre target tables.

In particular, the following Analysator values are calibration choices / engineering heuristics rather than values directly quoted from the RoEx publication:
- exact LUFS min/max ranges for each Analysator genre
- exact PLR min/max ranges for each genre
- exact LRA min/max ranges for each genre
- scoring margins and weights
- exact tonal-percent min/max ranges
- the Modern versus Vintage split
- the final score thresholds Excellent / Good / Attention / Critical
- the 60/40 technical-versus-style overall weighting

The RoEx study's compression metric is also not identical to Analysator's current PLR/LRA model. RoEx describes dynamic range using 20*log10(maximum / mean amplitude), compared with empirical genre-specific averages. Analysator currently uses PLR and LRA as musically meaningful dynamics descriptors. Therefore the RoEx findings validate the need for genre-aware dynamics assessment, but do not directly validate Analysator's exact PLR/LRA numeric ranges.

Likewise, the paper describes tonal analysis using the same four broad frequency bands, but its published results are categorical / distributional rather than a table of exact target energy percentages. Analysator's exact tonal percentage ranges are therefore derived calibration values, not direct RoEx thresholds.

## Era handling

The RoEx dataset/paper does not define a Modern versus Vintage calibration. Analysator's `Era::Modern` / `Era::Vintage` distinction is an independent product feature and must be treated as a separate heuristic calibration layer.

## Data limitations inherited from the reference study

The study itself notes that genre and MIX/MASTER status were selected by users, so misclassification can affect distributions. It also states that compression, tonal characteristics and stereo-field interpretations reflect empirical practice and current industry trends, not immutable standards.

For Analysator this means:
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

### Paper-to-code directional checks

The current profile directions agree with the published RoEx trends in several important places:
- General MIX includes approximately -23 LUFS comfortably inside its accepted loudness range, matching the study's observed MIX density peak around -23 LUFS.
- General MASTER includes approximately -14 LUFS comfortably inside its accepted loudness range, matching the study's observed MASTER clustering around -14 LUFS.
- MIX profiles preserve wider dynamic ranges than dense modern MASTER profiles, consistent with the study's finding that undercompression is much more prevalent in mixes and masters are generally more tightly controlled.
- Techno / House-EDM tonal profiles allow substantially more low-band energy than Acoustic/Folk, consistent with the published genre trend.
- Metal permits more high-frequency energy than several mainstream profiles, consistent with the paper's observation of high-frequency peaks in metal.
- Technical clipping, phase and mono checks are intentionally genre-independent. This matches the distinction between universal technical faults and genre-dependent aesthetics.

No production scoring constant needs to be changed merely to make these qualitative findings line up; the architecture already does.

### Important caution: loud masters

The RoEx dataset shows that a large majority of masters are louder than -14 LUFS, and the paper ranks `too loud` as the most common master issue. This does **not** mean a genre-style score should automatically reject every master above -14 LUFS. The dataset describes submitted music, not a verified corpus of ideal masters, and loudness normalization itself is not a technical defect.

Analysator therefore keeps three concepts separate:
1. genre/style plausibility,
2. PCM technical safety,
3. streaming delivery compatibility.

This separation is important because a loud modern Metal/EDM master can be stylistically plausible while still needing adequate true-peak headroom for streaming encoding.

## Streaming reference check

Spotify's current artist guidance remains consistent with Analysator's streaming headroom rule:
- normal playback normalization target: approximately -14 LUFS;
- recommended maximum True Peak: -1 dBTP for masters at or below -14 LUFS;
- if the master is louder than -14 LUFS, Spotify recommends keeping True Peak below -2 dBTP to reduce encoding distortion risk.

That matches the current `AssessmentModel.cpp` branch that uses a -2 dBTP recommendation for masters louder than -14 LUFS and -1 dBTP otherwise. The calculated `streamingGainDb = -14 - integratedLufs` is therefore a useful normalization estimate, not a command to remaster every track to exactly -14 LUFS.

## Calibration conclusion as of 2026-09-06

The current production scoring should remain unchanged for now.

Reason:
- the RoEx/AES evidence supports the structure and direction of the Analysator model;
- the exact per-genre LUFS/PLR/LRA/tonal boundaries are not directly published as authoritative targets;
- RoEx compression is not the same metric as PLR/LRA;
- the source dataset contains both good and problematic submissions, so empirical frequency must not be mistaken for a quality target;
- current streaming True Peak handling agrees with Spotify's published delivery guidance.

The main remaining calibration risk is therefore not an obvious contradiction with the RoEx data, but the precision of Analysator's own derived per-genre ranges. Those should only be narrowed or shifted when a reproducible reference distribution or additional trusted mastering references justify it.

## Next calibration step

Where practical, process the public Zenodo dataset to calculate genre-specific distributions for fields that map directly to Analysator (especially integrated loudness, true peak, clipping category and tonal-band category). Treat compression only as a directional cross-check because the source metric differs from PLR/LRA.

Any future mapping should explicitly distinguish:
- directly observed source statistics,
- derived Analysator calibration,
- independent engineering / delivery safety rules.

No scoring constants should be changed merely to make Analysator imitate the most common values in the dataset. The dataset contains many problematic mixes/masters and describes what users submitted, not a corpus of verified reference masters. It is best used to establish realistic distributions, prevalence and genre trends, while technical recommendations and delivery safety remain grounded in metering standards and engineering practice.
