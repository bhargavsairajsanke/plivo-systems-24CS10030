# Experiment Log

- **Profile A | Delay 100ms:** INVALID. Miss rate 1.20%, Overhead 1.11x. 18 packets missed the deadline due to NACK round-trip times slightly exceeding 100ms.
- **Profile A | Delay 120ms:** VALID. Miss rate 0.20%, Overhead 1.11x. Increased delay allowed adequate time for retransmissions to arrive.
- **Profile B | Delay 250ms:** VALID. Miss rate 0.27%, Overhead 1.37x. System easily handles 5% loss and jitter.
- **Profile B | Delay 210ms:** INVALID. Miss rate climbed over 1.00% as the 210ms window was too tight for worst-case round-trip NACKs.
- **Profile B | Delay 230ms:** VALID. Locked in as the lowest safe threshold for Moderate network conditions.
