# RoEx raw dataset audit for Analysator

Source dataset: Zenodo DOI 10.5281/zenodo.13683186 (`dataset.csv`).

## Raw data checks

- Rows: 218,109
- Columns: 18
- User-labelled MIX rows: 67,838
- User-labelled MASTER rows: 150,271
- Musical styles: 30
- Integrated loudness contains 202 non-finite values (`-inf`) and 51 finite values above 0 LUFS. These 253 rows were excluded from the loudness distribution audit.
- No rows were below -70 LUFS after finite-value filtering.

The cleaned loudness audit therefore used 217,856 rows.

## Important limitation

The dataset is a corpus of submitted MixCheck Studio analyses, not a corpus of verified reference productions. It contains good, mediocre and defective material. Raw medians therefore describe practice, not targets.

RoEx's `OPTIMAL` loudness labels also cluster in a relatively narrow, almost genre-independent region. They must not be interpreted as genre-specific commercial loudness targets for Analysator.

## Mapping used for exploratory comparison

The mapping below is deliberately broad and is only for statistical comparison:

- Rock: rock, indie_rock, punk
- Metal: metal
- Pop: pop, indie_pop
- Techno: techno, trance, drum_n_bass
- House / EDM: house, electronic
- Hip-Hop / Trap: hip-hop, hip_hop_grime, trap, rnb
- Electronic / Ambient: ambient, lo_fi, experimental, instrumental
- Acoustic / Folk: acoustic, folk, country, blues
- Jazz: jazz, soul, funk
- Classical: orchestral
- Cinematic: orchestral, instrumental (heuristic overlap; interpret cautiously)
- General: all 30 styles

This mapping is not asserted to be a one-to-one semantic equivalence.

## Cleaned integrated-loudness distributions

Values below are P10 / median / P90 in LUFS.

| Analysator group | MIX | MASTER |
|---|---:|---:|
| Rock | -23.1 / -17.5 / -10.6 | -15.5 / -11.3 / -7.6 |
| Metal | -21.5 / -15.7 / -8.4 | -14.9 / -10.0 / -6.8 |
| Pop | -23.0 / -17.2 / -10.0 | -15.0 / -10.8 / -7.9 |
| Techno | -22.3 / -17.2 / -9.3 | -14.2 / -9.9 / -7.1 |
| House / EDM | -23.4 / -17.2 / -8.7 | -14.6 / -10.2 / -7.4 |
| Hip-Hop / Trap | -21.8 / -15.8 / -9.5 | -15.5 / -11.4 / -8.4 |
| Electronic / Ambient | -26.6 / -19.2 / -11.5 | -17.0 / -13.0 / -9.0 |
| Acoustic / Folk | -24.3 / -17.8 / -10.9 | -18.3 / -13.6 / -9.5 |
| Jazz | -24.3 / -19.1 / -11.9 | -17.1 / -13.7 / -9.4 |
| Classical | -26.5 / -18.2 / -11.7 | -17.1 / -12.3 / -10.3 |
| Cinematic | -24.8 / -18.4 / -12.0 | -17.3 / -12.5 / -10.0 |
| General | -23.0 / -17.0 / -9.6 | -15.6 / -11.1 / -7.9 |

The expected high-level relationship is clearly present: MIX is substantially more dispersed and typically quieter than MASTER, while dense electronic/metal genres trend louder than more open acoustic/ambient material.

## Coverage of current Analysator LUFS ranges

Percentage of cleaned raw observations falling inside the current production LUFS ranges:

| Group | MIX profile | MASTER modern | MASTER vintage |
|---|---:|---:|---:|
| Rock | 71.0% | 81.5% | 74.9% |
| Metal | 66.8% | 67.7% | 74.8% |
| Pop | 70.3% | 88.2% | 73.5% |
| Techno | 70.5% | 80.4% | 70.8% |
| House / EDM | 63.3% | 77.8% | 75.2% |
| Hip-Hop / Trap | 70.6% | 78.6% | 81.1% |
| Electronic / Ambient | 78.8% | 90.6% | 82.0% |
| Acoustic / Folk | 67.3% | 85.6% | 66.7% |
| Jazz | 75.3% | 83.6% | 68.0% |
| Classical | 60.2% | 33.9% | 33.9% |
| Cinematic | 77.8% | 93.9% | 93.9% |
| General | 71.4% | 88.7% | 63.4% |

Interpretation:

- Most current MIX and modern MASTER ranges overlap a large majority of real-world submissions despite not having been fitted directly to this CSV.
- This is useful directional validation, not proof that the ranges are optimal.
- Classical is the obvious outlier: only 33.9% of orchestral MASTER submissions lie inside the current -27 to -13 LUFS range because the raw orchestral median is about -12.3 LUFS. This must NOT be fixed blindly: the corpus contains submitted masters, not curated classical references, and the RoEx own loudness classifier is not genre-specific enough to resolve this by itself.

## RoEx `OPTIMAL` loudness subset

When only rows labelled `OPTIMAL` by the RoEx/MixCheck loudness classifier are examined, MASTER medians for virtually every mapped genre cluster around -17 LUFS, while MIX medians cluster around roughly -20 LUFS.

Examples of `OPTIMAL` MASTER medians:

- Rock: -16.96 LUFS
- Metal: -16.88 LUFS
- Pop: -16.89 LUFS
- Techno: -17.13 LUFS
- House / EDM: -17.01 LUFS
- Hip-Hop / Trap: -16.94 LUFS
- Acoustic / Folk: -16.87 LUFS
- Jazz: -17.02 LUFS
- Classical: -17.13 LUFS

This near-uniformity demonstrates why the `OPTIMAL` labels should not be used as Analysator's genre-style targets. They appear to encode a general loudness recommendation rather than a descriptive per-genre mastering distribution.

## Dynamics information

The public CSV does not contain the raw numeric compression descriptor needed to derive Analysator PLR or LRA targets. It contains categorical DRC results (`LESS`, `OPTIMAL`, `MORE`). Therefore this dataset cannot directly validate the numerical PLR/LRA min/max values currently in `AssessmentModel.cpp`.

The categorical distributions do, however, confirm strong genre dependence. For example:

- Metal MASTER: 51.9% `OPTIMAL`, 42.0% `MORE`, 6.1% `LESS`
- Techno MASTER: 40.2% `OPTIMAL`, 26.4% `MORE`, 33.3% `LESS`
- Jazz MASTER: 49.5% `OPTIMAL`, 8.6% `MORE`, 41.9% `LESS`
- Orchestral/Classical MASTER: 2.3% `OPTIMAL`, 0.1% `MORE`, 97.6% `LESS`

This confirms that one universal dynamics profile would be inappropriate, but it does not supply PLR/LRA thresholds.

## Tonal categories

The CSV supplies categorical LOW/MEDIUM/HIGH results for four broad tonal bands, not raw percentages. This can validate direction but cannot produce exact Analysator percentage limits.

Observed tendencies in the mapped corpus are consistent with the current genre-aware concept:

- Techno / House / Hip-Hop show less frequent LOW-bass classifications than Rock/Metal/Acoustic/Classical.
- Electronic/Ambient has the highest incidence of HIGH-bass labels in this broad mapping.
- Acoustic/Classical/Cinematic have substantially more LOW-high-band classifications than Techno/House/Hip-Hop.
- Low-mid and high-mid classifications are overwhelmingly MEDIUM for most submitted material, so these labels carry less discriminatory information than the bass/high bands.

## Stereo / phase trends

The raw dataset also confirms that mono compatibility and phase behaviour vary with style and submitted material. Examples:

- Metal MASTER mono-compatible: 95.3%; phase issues: 19.8%
- Techno MASTER mono-compatible: 93.8%; phase issues: 23.6%
- Hip-Hop / Trap MASTER mono-compatible: 87.0%; phase issues: 5.3%
- Acoustic / Folk MASTER mono-compatible: 75.3%; phase issues: 10.4%
- Orchestral/Classical MASTER mono-compatible: 82.1%; phase issues: 39.2%

These figures describe prevalence only. Analysator should continue to treat severe phase/mono faults as technical conditions rather than normalize them because they are common in a genre.

## Calibration decision after raw-data audit

Do not change production scoring constants yet.

The raw CSV gives useful empirical support for:

1. separate MIX and MASTER profiles;
2. genre-aware interpretation;
3. broad relative loudness ordering between dense/electronic and open/acoustic material;
4. genre-aware dynamics and tonal interpretation;
5. keeping technical safety independent from stylistic prevalence.

It does NOT directly provide:

- PLR target ranges;
- LRA target ranges;
- exact tonal percentages;
- Modern/Vintage calibration;
- authoritative genre-specific loudness targets.

The next calibration work should therefore focus on external/reference-grade evidence for PLR/LRA and the Modern/Vintage layer, while keeping the present production ranges unchanged until a stronger basis exists.
