# Transport-aware analysis note (2026-09-06)

Observed in Studio One: while ANALYZE is active, the verdict can change at the physical end of the song and then the displayed result can disappear before FINALIZE is pressed.

The processor previously analysed every process callback while AnalysisState::Live, regardless of the host transport state. Some hosts continue to call process() with stopped/tail/silent blocks at transport end. Those blocks must not extend the programme measurement.

Required invariant:
- ANALYZE clears and arms a new measurement.
- Audio contributes only while the host transport reports kPlaying, when a ProcessContext is available.
- A transition to stopped must freeze the accumulated programme history in place without resetting it.
- FINALIZE freezes the same accumulated history.
- RESET or a new ANALYZE is the only normal user action that clears the measurement.
- If ProcessContext is unavailable, retain the previous fallback behaviour so hosts that do not provide transport context still analyse.
