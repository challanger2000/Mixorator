# Result lifecycle audit (2026-09-06)

Scope: disappearing LIVE/FINAL result at programme end in Studio One. This audit traces every code path that can clear or invalidate the visible result and cross-checks the VST3 Data Exchange lifecycle.

## Verified controller clear paths

`Controller::hasPacket_` is only cleared by:
- controller initialization (new/reinitialized controller instance),
- explicit ANALYZE,
- explicit RESET.

`queueClosed()` does not clear the packet. Editor close/open only clears/rebinds view pointers and must preserve the packet.

Therefore an already valid result disappearing without ANALYZE/RESET cannot be caused merely by receiving a later normal Data Exchange packet after the controller-side last-valid guard. A controller reinitialization/recreation or an equivalent lifecycle rebuild remains a distinct path that must be handled defensively.

## Concrete gaps found

1. The earlier transport-end investigation explicitly required a new packet-sequence epoch after every Data Exchange queue reopen, but the implementation did not do this. The controller compared a new queue's first packet against the old queue's sequence and could reject it forever if the producer sequence restarted.
2. While FINAL was pending, `evaluateLatest()` deliberately marked loudness/PLR/LRA unavailable. This could blank a previously valid result between pressing FINALIZE and receiving the definitive snapshot.
3. A failed/malformed definitive snapshot request was remembered as already requested and therefore had no retry path until another state transition.
4. LIVE assessment still uses rolling Short-Term loudness while programme material is present and only falls back to Integrated Loudness after Short-Term becomes unavailable. This explains LIVE verdict movement such as ATTENTION -> EXCELLENT near the end; it is separate from the disappearance bug.
5. Processor robustness tests cover repeated same-rate `setupProcessing()` but there is no controller/Data Exchange regression test for queue close/open, sequence restart, controller recreation, or FINAL snapshot retry.
6. A genuine sample-rate change resets processor analysis to IDLE but there is currently no explicit IDLE packet/state message to clear a stale controller result. This is not the observed song-end path, but it is a lifecycle consistency gap.

## Hardening applied after audit

- A queue reopen now starts a new packet-sequence epoch; the first valid packet is accepted regardless of the previous queue sequence.
- The last valid metrics are preserved when a later lifecycle packet lacks loudness data.
- FINAL/PENDING keeps the previous valid assessment visible instead of forcing N/A.
- FINAL snapshot requests are retried on editor/queue reopen and malformed snapshot replies reset the retry guard.
- Queue close/open never clears the visible result.

## VST3 / Studio One lifecycle context

Steinberg's Data Exchange helper closes its queue on processor deactivation and opens it again on activation. Studio One's Plug-in Nap can stop plug-in processing when no audio is passed, so programme-end silence is a real activation/queue-lifecycle boundary that this plug-in must survive without losing the measurement.

## Required invariant

Only explicit ANALYZE/RESET (and a genuine measurement-environment change such as sample-rate change) may invalidate accumulated programme data. Transport stop, silence, Plug-in Nap, queue close/open, editor close/open, and same-rate processing reconfiguration must leave the last valid result visible and finalizable.
