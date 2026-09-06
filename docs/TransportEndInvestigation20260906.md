# Transport-end investigation (2026-09-06)

Observed in Studio One while ANALYZE is active and the song reaches its end: the provisional verdict can change (for example EXCELLENT to GOOD) and then the visible analysis can disappear.

Lifecycle audit after commit a3274455 found two remaining restart-sensitive paths:

1. `Processor::setupProcessing()` unconditionally called `analysis_.prepare()`, which resets the complete accumulated programme analysis and analysis state. Hosts are allowed to call setupProcessing more than once during an instance lifetime.
2. DataExchange packet sequence numbers restart when processing setup is rebuilt. The controller previously treated any lower sequence as stale forever, even after the DataExchange queue had been reopened.

Required semantics:
- ANALYZE and RESET are the only normal user actions that clear accumulated programme data.
- Host activation/deactivation, DataExchange queue close/open, editor close/open, and repeated setup with an unchanged sample rate must preserve the current analysis.
- A queue reopen establishes a new packet-sequence epoch, so the first packet after reopen must be accepted even if its sequence is lower than the previously displayed packet.
- A genuine sample-rate change requires re-preparing the DSP engine because its filters/windows depend on sample rate; that is treated as a new measurement environment.
