# Genre coverage audit

## Purpose

This audit compares Analysator's user-facing genre taxonomy with the 30 `musical_style` labels in the RoEx/AES dataset (218,109 submissions). It is a calibration audit, not a proposal to blindly expose all 30 RoEx labels in the UI.

## Current Analysator genres

Rock, Metal, Pop, Techno, House / EDM, Hip-Hop / Trap, Electronic / Ambient, Acoustic / Folk, Jazz, Classical, Cinematic Music, General.

## RoEx source labels and counts

- electronic 34,244
- hip_hop_grime 31,616
- rock 21,017
- pop 20,921
- house 15,281
- trap 14,917
- metal 8,750
- techno 5,956
- indie_rock 5,384
- rnb 4,902
- hip-hop 4,809
- experimental 4,255
- acoustic 4,228
- ambient 3,915
- drum_n_bass 3,822
- indie_pop 3,723
- lo_fi 3,511
- afrobeat 3,137
- punk 2,951
- orchestral 2,757
- jazz 2,543
- folk 2,452
- latin 2,434
- trance 1,993
- funk 1,874
- reggae 1,845
- soul 1,690
- country 1,225
- blues 1,072
- instrumental 885

## Coverage assessment

### Strong direct matches

The following Analysator choices have a clear RoEx counterpart and enough observations for useful empirical cross-checks:

- Rock <- rock; indie_rock can be treated as a related secondary population.
- Metal <- metal.
- Pop <- pop; indie_pop can be treated as a related secondary population.
- Techno <- techno.
- House / EDM <- house, with trance and selected electronic styles as secondary comparisons rather than automatic equivalents.
- Hip-Hop / Trap <- hip_hop_grime, hip-hop, trap.
- Jazz <- jazz.
- Acoustic / Folk <- acoustic, folk; country can be a secondary comparison.

### Valid broad groups, but heterogeneous

- Electronic / Ambient currently spans two very different ideas. `electronic` is the largest RoEx label (34,244), while `ambient` has 3,915 entries. A single profile must therefore be deliberately broad. It should not be presented as if RoEx supplied one unified Electronic/Ambient target.
- House / EDM is a useful user-facing umbrella, but RoEx separately labels house, techno, trance and drum_n_bass. The Analysator profile should not assume all EDM subgenres share identical dynamics or tonal balance.
- Hip-Hop / Trap is defensible as a broad selection, but its empirical population combines several separately labelled RoEx styles.

### Weak or indirect source mapping

- Classical has no RoEx `classical` label. `orchestral` (2,757) is the nearest source label, but orchestral material is not synonymous with classical music.
- Cinematic Music has no direct RoEx `cinematic`, `film_score`, or `soundtrack` label. Using `orchestral` as evidence can support some orchestral-cinematic tendencies, but cannot empirically validate Cinematic Music as a whole. Modern hybrid cinematic music can contain electronic, rock/metal, percussion and sound-design characteristics that are not represented by the orchestral label alone.
- General is intentionally not a genre and should remain a broad fallback rather than a pseudo-reference population.

## Important omissions in the current user-facing taxonomy

Not every RoEx label deserves its own Analysator menu item. However, several sizeable musical families are currently forced into a less suitable choice:

1. **R&B / Soul**: rnb (4,902) + soul (1,690), with funk (1,874) as a useful related comparison. This is a coherent and common family that is not well represented by Pop or Hip-Hop / Trap alone.
2. **Drum & Bass**: 3,822 direct RoEx entries. Its bass distribution and dynamics can differ materially from generic House / EDM and Ambient/Electronic. It is a good candidate for a dedicated electronic subgenre if the menu can tolerate one more item.
3. **Country / Blues**: country (1,225) + blues (1,072). This is smaller than R&B/Soul but has a distinct acoustic/roots production tradition. It can remain under Acoustic/Folk if simplicity is prioritized.
4. **Reggae**: 1,845 direct entries and a distinctive low-frequency/rhythmic balance. Dedicated support is optional; General is safer than pretending it is House/EDM.
5. **Latin / Afrobeat**: latin (2,434) + afrobeat (3,137). These are not one genre and should not be merged into one calibration merely to increase sample count. They demonstrate that the current menu does not cover every contemporary style directly.

## Recommended product taxonomy

Keep the menu compact. The current core is broadly useful, but the strongest improvement would be:

- keep Rock
- keep Metal
- keep Pop
- keep Techno
- keep House / EDM
- add **Drum & Bass**
- keep Hip-Hop / Trap
- add **R&B / Soul**
- split **Electronic / Ambient** into **Electronic** and **Ambient** if profile calibration can support both; otherwise retain the broad label but explicitly use a wide tolerance
- keep Acoustic / Folk
- keep Jazz
- keep Classical, but calibrate it from independent classical references rather than claiming direct RoEx validation
- keep Cinematic Music, but calibrate Modern/Vintage independently; RoEx `orchestral` alone is insufficient
- keep General as fallback

Country/Blues, Reggae, Latin, Afrobeat, Punk and other styles do not need to become menu entries immediately. Related populations can inform broad profiles, while General provides a safe fallback.

## Modern / Vintage finding

The RoEx dataset contains no release year or era field. It cannot directly establish Modern versus Vintage thresholds. Era calibration therefore needs independent engineering/reference evidence. This is especially important for Cinematic and Classical, which are currently era-neutral in `AssessmentModel.cpp`.

For Cinematic, a real Modern/Vintage distinction is musically plausible, but it must not be fabricated from the RoEx `orchestral` subset. Modern hybrid cinematic masters can be substantially denser/louder and more bass-heavy than traditional orchestral scores, while traditional/vintage-oriented scoring can preserve substantially more dynamics.

## Data-use rules

1. Do not equate a broad Analysator genre with a union of RoEx labels without checking each component distribution.
2. Do not use RoEx frequency of occurrence as a quality target; the dataset contains problematic submissions.
3. Use RoEx for empirical distributions, prevalence and directional genre trends.
4. Use metering standards and engineering practice for technical safety.
5. Use independent curated references where RoEx lacks the required concept, especially PLR/LRA, Classical, Cinematic and Modern/Vintage.
6. Keep style deviations advisory; do not turn genre calibration into a technical fault.

## Implementation status

No production scoring constants or GUI controls were changed by this audit. The current known-good analysis/UI milestone remains untouched. Any taxonomy/profile change should be made only after the new candidate profiles and tests are prepared together, so the user needs one consolidated build rather than repeated GUI/build cycles.
