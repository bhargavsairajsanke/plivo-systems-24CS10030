1. The design implements a Selective-Repeat ARQ (Automatic Repeat reQuest) system over UDP.
2. The sender maintains a history buffer of transmitted packets, listening simultaneously for new frames and NACKs (Negative Acknowledgements) using non-blocking `poll()`.
3. The receiver forwards packets directly to the player to minimize playout delay, avoiding the need for a complex jitter buffer.
4. The receiver continuously scans a sliding window of recent sequence numbers and aggressively sends NACKs for missing frames before their deadline expires.
5. This strategy was chosen because Forward Error Correction (100% duplication) strictly violates the 2.0x bandwidth cap, whereas ARQ averages ~1.1x overhead.
6. We should be graded at a delay_ms of 230.
7. This architecture breaks when burst losses exceed the round-trip time required for a NACK and retransmission before the strict 230ms frame deadline.
