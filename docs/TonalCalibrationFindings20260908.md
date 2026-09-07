# Tonal calibration findings — 2026-09-08

## Scope

This note records the first real-song calibration pass for the experimental eight-band tonal measurement. The production evaluator remains unchanged. The purpose is to decide what representation is defensible before detailed tonal information is allowed to influence verdicts.

Bands:

1. Sub: 20–80 Hz
2. Bass: 80–250 Hz
3. Low-mid body: 250–500 Hz
4. Mid: 500–2000 Hz
5. Presence: 2–5 kHz
6. Upper presence: 5–8 kHz
7. Brilliance: 8–12 kHz
8. Air: 12–20 kHz

## Measured anchors

| Reference | Context | Sub | Bass | Low-mid | Mid | Presence | Upper presence | Brilliance | Air |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Glow Remix | House/EDM, Modern | 41.4372 | 36.8340 | 9.2006 | 9.9428 | 1.3891 | 0.6383 | 0.4488 | 0.1093 |
| Ricky Nelson — A Teenager's Romance | Pop, Vintage | 0.6874 | 25.6551 | 21.2383 | 41.9215 | 9.4837 | 0.7246 | 0.2507 | 0.0386 |
| Winnetou | Cinematic, Vintage | 0.4809 | 15.2061 | 32.3142 | 46.0280 | 5.7695 | 0.1649 | 0.0310 | 0.0053 |
| Tchaikovsky/Bernstein — Valse des Fleurs | Classical, Modern | 2.2699 | 18.2910 | 27.6430 | 42.7908 | 8.7523 | 0.1840 | 0.0396 | 0.0293 |
| Israel Kamakawiwo'ole — Somewhere Over The Rainbow | Acoustic/Folk, Modern | 0.0342 | 20.5488 | 70.7207 | 7.2947 | 0.7576 | 0.3366 | 0.2462 | 0.0611 |

All rows sum to approximately 100%, confirming the detailed measurement transport is internally consistent.

## What the data disproves

The original seed envelopes implicitly treated raw FFT-energy percentages as if perceptually important upper bands should occupy several percent of total energy. The measured references contradict that assumption. Healthy commercial material can have only hundredths or tenths of a percent in 8–20 kHz because unweighted squared-amplitude energy is naturally dominated by lower frequencies.

Therefore a raw percentage in one upper band must not be interpreted directly as a perceptual amount of brightness or air. In particular, a tiny Air or Brilliance percentage is not evidence of a dull master by itself.

The Israel reference also demonstrates that a musically valid acoustic recording can concentrate more than 70% of raw energy in 250–500 Hz. A narrow universal target curve would misclassify such material.

## Decision

Do **not** integrate the current eight independent raw-percentage range score into production `evaluate()`.

Keep the raw eight-band percentages as objective measurements, but base the next calibration layer primarily on relationships between adjacent/related bands and logarithmic ratios. Candidate features:

- Sub / Bass balance
- Low-mid body / Mid balance
- Presence relative to Mid
- Upper presence / Presence
- Brilliance / Upper presence
- Air / Brilliance
- Low-frequency group (20–250) relative to body+mid (250–2000)
- High-frequency group (2–20k) relative to body+mid

Use `10 * log10((A + epsilon) / (B + epsilon))` for energy ratios. This compresses the enormous numerical span while preserving physically meaningful spectral relationships. The epsilon must be fixed and small enough not to dominate valid measured bands.

## Guardrails

- Genre and era may alter interpretation, never the measured percentages.
- No single reference track defines a target curve.
- Tonal balance is one component of style/mastering assessment, not a technical-fault detector by itself.
- Low upper-band raw energy alone is not a defect.
- Broad spectral tendencies should be evaluated jointly; avoid eight independent pass/fail gates.
- Production verdicts remain unchanged until ratio features are regression-tested against both synthetic signals and a wider reference set.
- Existing four-band production tonal scoring should be reviewed separately because it is derived from the same raw-energy representation; do not silently replace it during the experimental detailed-band work.

## Next implementation step

Add an experimental detailed-tonal feature extractor that converts the eight measured percentages into stable log-ratio features. Expose it only to calibration tests first. Anchor those tests with the five measured references above, then add deliberately altered synthetic/reference-like spectra to verify that the features react in the expected direction. Only after that should scoring envelopes be designed.
