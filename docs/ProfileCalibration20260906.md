# Profile calibration pass — 2026-09-06

## Scope

This pass follows the full raw-data audit of the RoEx/AES dataset (218,109 submitted mixes/masters) and a separate standards/literature check. The goal is to improve genre coverage without pretending that observed submission distributions are authoritative mastering targets.

No GUI geometry, resize behavior, analysis state machine, technical-safety scoring, PCM delivery scoring, or streaming headroom logic is changed in this pass.

## Evidence hierarchy

1. **Standards / measurement definitions**
   - ITU-R BS.1770 / EBU R128 family for loudness measurement.
   - EBU Tech 3342 for Loudness Range (LRA).
   - AES guidance and literature for PLR/PSR terminology and the distinction between loudness, headroom and dynamics.

2. **Empirical trend data**
   - RoEx / AES 157 raw dataset, DOI 10.5281/zenodo.13683186.
   - The dataset describes real user submissions. It is used to validate direction, coverage and genre differences, not to define a single “correct” target.

3. **Derived Analysator heuristics**
   - Exact per-genre PLR/LRA corridors and tonal-energy percentages remain product calibration parameters.
   - RoEx does not expose raw PLR or LRA values, and its tonal fields are categorical rather than exact band-energy percentages.
   - `Modern` / `Vintage` is not present in the RoEx dataset and therefore remains a deliberately soft heuristic.

## Genre set after this pass

The user-facing set is expanded from 12 to 15 entries:

- Rock
- Metal
- Pop
- Techno
- House / EDM
- Drum & Bass
- Hip-Hop / Trap
- R&B / Soul
- Electronic
- Ambient
- Acoustic / Folk
- Jazz
- Classical
- Cinematic Music
- General

### Why these additions

**Drum & Bass**
- RoEx has 3,822 direct `drum_n_bass` submissions.
- Its loudness and low-frequency behavior justify a profile separate from generic House/EDM.

**R&B / Soul**
- RoEx has 4,902 `rnb`, 1,690 `soul`, plus 1,874 `funk` entries.
- This is a sufficiently large and coherent family not to hide under Pop.

**Electronic vs Ambient**
- RoEx contains 34,244 `electronic` and 3,915 `ambient` entries.
- Ambient is materially quieter and more dynamic in the raw loudness distribution than generic Electronic, so the previous combined `Electronic / Ambient` profile was too coarse.

## Mapping used for the raw-data comparison

- Rock: `rock`, `indie_rock`, `punk`
- Metal: `metal`
- Pop: `pop`, `indie_pop`
- Techno: `techno`
- House / EDM: `house`, `trance`
- Drum & Bass: `drum_n_bass`
- Hip-Hop / Trap: `hip_hop_grime`, `hip-hop`, `trap`
- R&B / Soul: `rnb`, `soul`, `funk`
- Electronic: `electronic`, `experimental`, `lo_fi`
- Ambient: `ambient`
- Acoustic / Folk: `acoustic`, `folk`, `country`, `blues`
- Jazz: `jazz`
- Classical: RoEx `orchestral` is used only as an imperfect proxy
- Cinematic: no direct RoEx category exists; `orchestral` / `instrumental` are only directional proxies and are not treated as proof
- General: fallback / intentionally broad

The remaining RoEx categories such as reggae, latin and afrobeat are useful calibration context but do not justify expanding the main menu into an exhaustive genre catalogue at this stage.

## Raw loudness sanity check

After excluding non-finite and impossible positive LUFS values, the following representative medians were observed:

| Family | Mix median | Master median |
|---|---:|---:|
| Rock | -17.5 | -11.3 |
| Metal | -15.7 | -10.0 |
| Pop | -17.2 | -10.8 |
| Techno | -17.3 | -9.7 |
| House / EDM | -19.2 | -10.1 |
| Drum & Bass | -17.7 | -10.0 |
| Hip-Hop / Trap | -15.7 | -11.4 |
| R&B / Soul | -17.8 | -12.1 |
| Electronic | -16.6 | -10.6 |
| Ambient | -18.5 | -13.4 |
| Acoustic / Folk | -17.8 | -13.6 |
| Jazz | -19.6 | -13.9 |
| Orchestral proxy | -18.2 | -12.3 |

These medians are **descriptive**, not mastering targets.

## New / revised profile logic

### Drum & Bass
Derived from the RoEx DnB distribution and the existing dense-electronic profiles:
- Mix remains broad enough for pre-master headroom.
- Modern Master allows dense/loud material similar to Techno but with a slightly broader low-frequency tonal window.
- Vintage is a softer, more dynamic alternative.

### R&B / Soul
Placed between Pop / Hip-Hop and the more dynamic acoustic families:
- Modern accepts contemporary, moderately dense masters without requiring Metal/EDM-style density.
- Vintage preserves more PLR/LRA and lower loudness.

### Electronic
The former Electronic/Ambient compromise is replaced with a dedicated general-electronic profile:
- Modern is allowed to be denser than before.
- Vintage remains broader and more dynamic.

### Ambient
Gets its own substantially wider dynamic profile:
- lower loudness floor,
- higher PLR/LRA acceptance,
- broader and less bass-prescriptive tonal corridor.

### Cinematic
RoEx cannot validate Cinematic directly. The previous era-neutral MASTER profile is therefore replaced by a **soft era distinction**, not an alleged standard:
- **Modern Cinematic** covers contemporary hybrid/trailer/score production that can be denser and louder.
- **Vintage Cinematic** favors greater peak-to-loudness ratio and loudness range.

MIX remains era-neutral, consistent with the rest of the architecture.

### Classical
Classical remains era-neutral. RoEx only provides an `orchestral` proxy, and professional AES guidance explicitly emphasizes that wide-dynamic-range/fine-arts material should not be reduced to a single numeric loudness target. The existing deliberately dynamic profile is therefore retained rather than overfitted to user-submitted orchestral masters.

## PLR / LRA caution

PLR is a recognized long-term dynamics descriptor, and LRA is a standardized supplementary loudness-range measure. However, authoritative sources do not provide reliable genre-by-genre “correct” PLR/LRA tables.

Therefore:
- Existing broad PLR/LRA corridors are retained where there is no contradiction.
- New genres inherit conservative ranges from adjacent styles plus empirical loudness/tonal trends.
- The score should treat these corridors as **plausibility ranges**, never as hard mastering requirements.

## Regression requirements added

The deterministic assessment suite now checks that:
- every profile for both modes and both eras returns finite scores,
- Modern and Vintage Cinematic are meaningfully differentiated,
- Classical remains era-neutral,
- Drum & Bass recognizes dense bass-forward material,
- Ambient is meaningfully separated from generic Electronic,
- a representative modern R&B/Soul master is accepted,
- universal technical and delivery safety remains independent of genre/era.

## Decision

This pass improves coverage and removes the largest known genre-modeling weaknesses while deliberately avoiding false precision. The technical score remains standards/safety driven; genre and era only affect the style plausibility layer.
