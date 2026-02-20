
## Physical Model

* 3 buckets: **A**, **B**, **C**.
* Each bucket holds exactly **one half-frame** (75 kB).
* Frame = Upper half + Lower half.
* The camera produces half-frames in strict order:

```
Frame N Upper → Frame N Lower → Frame N+1 Upper → Frame N+1 Lower → ...
```

* The camera DMA cycles through the three buckets in round-robin order (A → B → C → A → …).
* If the next bucket in the cycle is **protected** (currently being used by TX), the camera **skips** it and advances to the next non-protected bucket. If only **one** bucket is available, the camera keeps overwriting that single bucket. If **two** are available, the camera cycles between them, skipping the protected one.
* Each overwrite of a bucket that held a previous half-frame causes a **frame drop** — the lost content means that frame can never be fully sent.
* Because of this, the mapping between frames and buckets is **not deterministic**. Which bucket holds which half-frame depends entirely on the runtime timing between camera writes and TX sends.

**Protection lifecycle of a TX pair:** When TX selects a pair, **both** buckets become protected immediately. After the Upper half is sent, its bucket is freed. The Lower bucket stays protected until TX finishes sending it.

## TX Scheduling Rule (must be stated explicitly)

TX never idles.
When a bucket finishes transmission, TX immediately advances to the next bucket.
TX does not wait for a full frame before starting.
If the paired half is incomplete, TX proceeds in sequence and will reach it when its turn comes.

### TX Pair Selection Rule

TX always sends in pairs (Upper half-frame first, then Lower half-frame).

**At startup:** TX selects A+B, because camera starts writing in A→B order
and no written data exists yet.

**After finishing a pair:** TX always selects the **two buckets that are NOT
the bucket TX just finished sending** (i.e., not the Lower-half bucket of
the old pair). With 3 buckets total and TX just finishing one, exactly two
candidates remain — those two become the new pair. There is no other
possibility.

**Order within the new pair:** whichever of the two buckets holds the Upper
half-frame is sent first; the other (holding the Lower half-frame) is sent
second.

**Why there is no ambiguity:** because neither camera nor TX ever idles or
waits, it is impossible for a complete next frame to already exist in the
two other buckets at the moment TX finishes a pair. The camera is always
mid-write or has just finished, so the two available buckets always contain
data from the current or recent camera output, and the Upper/Lower
assignment is unambiguous.

These rules are verified in Scenarios 1, 1b, and 2.

---

## Step-by-Step Scenario Walkthroughs

The following traces show the state of every bucket at each camera DMA write,
illustrating how the system behaves at different relative speeds.

**Notation:**
* TX REC = TX is sending this bucket AND camera is writing to it simultaneously (unprotected — we assume camera is always ahead)
* TXP = TX is actively sending this bucket, and it is protected (camera already finished writing this frame's data)
* PD = bucket is protected but NOT being sent — it's the queued partner of the currently-sending bucket (e.g., the Lower half while TX sends the Upper half)
* REC = camera is writing to this bucket
* FREE = bucket is free
* Frame labels on PD/TXP cells (e.g., F3U TXP, F5L PD) indicate what data the bucket holds


---

### Scenario 1 — Camera 2× faster than TX

Camera: 1 tick per half-frame. 
TX: 2 ticks per half-frame. Both start at tick 0.

| Tick | Camera writes |     A |     B |     C |                                           TX action |
|------|---------------|-------|-------|-------|-----------------------------------------------------|
|    0 |      F0U -> A | TX REC|  FREE |  FREE | TX starts sending A, which contains partial F0U     |
|    1 |      F0L -> B |   TXP |   REC |  FREE | TX is still sending A, it is protected now          |
|    2 |      F1U -> C |  FREE |   TXP |   REC | TX frees A. Starts sending B                        |
|    3 |      F1L -> A |   REC |   TXP |   F1U | TX finishes B, Camera finishes A                    |
|    4 |      F2U -> B | F1L PD|   REC |   TXP | TX starts C+A (protected), camera can use B and B only|
|    5 |      F2L -> B | F1L PD|   REC |   TXP | TX continues sending C. Camera overwrites B, since A is protected.|
|    6 |      F3U -> C |   TXP |   F2L |   REC | TX frees C, starts sending A. Camera immediately starts using C.  |
|    7 |      F3L -> B |   TXP |   REC |   F3U | TX finishes A,                                      |
|    8 |      F4U -> A |   REC | F3L PD|F3U TXP| TX starts to send C+B                               |
|    9 |      F4L -> A |   REC | F3L PD|F3U TXP| Only A free. Overwrite A. Frame 4 dropped.          |
|   10 |      F5U -> C |   F4L |F3L TXP|   REC | TX frees C. Camera immediately starts using it.      |
|   11 |      F5L -> A |   REC |F3L TXP|   F5U | F5 is ready, camera: C->A (free), TX frees B.       |
|   12 |      F6U -> B | F5L PD|   REC |F5U TXP| and so on and so on                                 |

**Result:** Frame 0 sent immediately at startup. 
Sent: F0U + F0L, F1U + F1L, F3U + F3L, F5U+(this is already tick 12)

---

### Scenario 1b — Camera 3× faster than TX

Camera: 1 tick per half-frame. 
TX: 3 ticks per half-frame. Both start at tick 0.

| Tick | Camera writes |     A |     B |     C |                                           TX action |
|------|---------------|-------|-------|-------|-----------------------------------------------------|
|    0 |      F0U -> A | TX REC|  FREE |  FREE | TX starts sending A (F0U), ticks 0-2                |
|    1 |      F0L -> B |   TXP |   REC |  FREE | TX sending A (tick 2/3)                             |
|    2 |      F1U -> C |   TXP |F0L PD |   REC | TX sending A (tick 3/3). B protected (F0L queued)   |
|    3 |      F1L -> A |   REC |   TXP |   F1U | TX finishes A. Starts sending B (F0L), ticks 3-5    |
|    4 |      F2U -> C |   F1L |   TXP |   REC | TX sending B (tick 2/3). Camera skips B, writes C   |
|    5 |      F2L -> A |   REC |   TXP |   F2U | TX sending B (tick 3/3)                             |
|    6 |      F3U -> B | F2L PD|   REC |   TXP | TX finishes B. Picks C+A (F2). Starts C, ticks 6-8  |
|    7 |      F3L -> B | F2L PD|   REC |   TXP | TX sending C (tick 2/3). Only B free, overwrites B  |
|    8 |      F4U -> B | F2L PD|   REC |   TXP | TX sending C (tick 3/3). Only B free, overwrites B  |
|    9 |      F4L -> C |   TXP |   F4U |   REC | TX finishes C. Starts sending A (F2L), ticks 9-11   |
|   10 |      F5U -> B |   TXP |   REC |   F4L | TX sending A (tick 2/3). Camera skips A, writes B   |
|   11 |      F5L -> C |   TXP |   F5U |   REC | TX sending A (tick 3/3)                             |
|   12 |      F6U -> A |   REC |   TXP | F5L PD| TX finishes A. Picks B+C (F5). Starts B, ticks 12-14|
|   13 |      F6L -> A |   REC |   TXP | F5L PD| TX sending B (tick 2/3). Only A free, overwrites A  |
|   14 |      F7U -> A |   REC |   TXP | F5L PD| TX sending B (tick 3/3). Only A free, overwrites A  |
|   15 |      F7L -> B |   F7U |   REC |   TXP | TX finishes B. Starts sending C (F5L), ticks 15-17  |
|   16 |      F8U -> A |   REC |   F7L |   TXP | TX sending C (tick 2/3). Camera skips C, writes A   |
|   17 |      F8L -> B |   F8U |   REC |   TXP | TX sending C (tick 3/3)                             |
|   18 |      F9U -> C |   TXP | F8L PD|   REC | TX finishes C. Picks A+B (F8). Starts A, ticks 18-20|

**Result:** Frame 0 sent at startup (F0U+F0L). Then Frame 2, Frame 5, Frame 8, Frame 11, ... Steady-state: every 3rd frame sent, 67% drop rate. The 6-tick TX cycle (3 ticks upper + 3 ticks lower) repeats with the pair rotating C+A -> B+C -> A+B -> C+A -> ...

---

### Scenario 2 — Camera same speed as TX

Camera: 1 tick per half-frame.
TX: 1 tick per half-frame. Both start at tick 0.

| Tick | Camera writes |     A |     B |     C |                                            TX action |
|------|---------------|-------|-------|-------|------------------------------------------------------|
|    0 |      F0U -> A | TX REC|  FREE |  FREE | TX starts sending A (F0U), camera writing A same time|
|    1 |      F0L -> B |  FREE | TX REC|  FREE | TX frees A. Starts sending B (F0L), camera writing B |
|    2 |      F1U -> C |  FREE |  FREE | TX REC| TX frees B. Frame 0 sent. Starts sending C (F1U)     |
|    3 |      F1L -> A | TX REC|  FREE |  FREE | TX frees C. Starts sending A (F1L), camera writing A |
|    4 |      F2U -> B |  FREE | TX REC|  FREE | TX frees A. Frame 1 sent. Starts sending B (F2U)     |
|    5 |      F2L -> C |  FREE |  FREE | TX REC| TX frees B. Starts sending C (F2L), camera writing C |
|    6 |      F3U -> A | TX REC|  FREE |  FREE | TX frees C. Frame 2 sent. Starts sending A (F3U)     |
|    7 |      F3L -> B |  FREE | TX REC|  FREE | TX frees A. Starts sending B (F3L), camera writing B |
|    8 |      F4U -> C |  FREE |  FREE | TX REC| TX frees B. Frame 3 sent. Starts sending C (F4U)     |

**Result:** 
Every frame sent, no frames dropped. TX and camera move in lockstep through
A->B->C->A->... Every tick is TX REC (camera and TX on the same bucket). TX
finishes each half in 1 tick so no bucket is ever held past camera's departure
— no TXP, no PD, no skips. Frames complete every 2 ticks (F0 at tick 1,
F1 at tick 3, F2 at tick 5, ...).

---

### Scenario 3 — Camera 2× slower than TX

Camera: 2 ticks per half-frame.
TX: 1 tick per half-frame. Both start at tick 0.

TX is faster than camera. At tick 0 (startup), TX sends while camera writes
(TX REC), producing partial data — TX DMA overtakes camera DMA within the
buffer. After tick 0, TX idles until each half-frame completes before sending.

| Tick | Camera writes    |       A |       B |       C |                                                 TX action |
|------|------------------|---------|---------|---------|-----------------------------------------------------------|
|    0 | F0U -> A (1/2)   |  TX REC |    FREE |    FREE | TX starts A (F0U partial — camera mid-write)              |
|    1 | F0U -> A (2/2)   |     REC |    FREE |    FREE | TX done with A. F0L not written yet. IDLE                 |
|    2 | F0L -> B (1/2)   |     F0U |     REC |    FREE | F0L being written. IDLE                                   |
|    3 | F0L -> B (2/2)   |     F0U |     REC |    FREE | F0L still being written. IDLE                             |
|    4 | F1U -> C (1/2)   |     F0U | F0L TXP |     REC | F0L complete. TX sends B                                  |
|    5 | F1U -> C (2/2)   |     F0U |    FREE |     REC | TX frees B. Frame 0 sent (F0U partial). IDLE              |
|    6 | F1L -> A (1/2)   |     REC |    FREE | F1U TXP | F1U complete. TX sends C                                  |
|    7 | F1L -> A (2/2)   |     REC |    FREE |    FREE | TX frees C. IDLE                                          |
|    8 | F2U -> B (1/2)   | F1L TXP |     REC |    FREE | F1L complete. TX sends A                                  |
|    9 | F2U -> B (2/2)   |    FREE |     REC |    FREE | TX frees A. Frame 1 sent. IDLE                            |
|   10 | F2L -> C (1/2)   |    FREE | F2U TXP |     REC | F2U complete. TX sends B                                  |
|   11 | F2L -> C (2/2)   |    FREE |    FREE |     REC | TX frees B. IDLE                                          |
|   12 | F3U -> A (1/2)   |     REC |    FREE | F2L TXP | F2L complete. TX sends C (F2L)                             |

**Result:**
All frames sent, no frames dropped. TX is 2× faster per half-frame but
effectively throttled to camera speed — it sends 1 tick, idles 1 tick,
repeating. Only F0U is broken (startup TX REC where TX overtakes camera).
From tick 4 onward, TX always sends completed buckets (TXP, clean data).
No pair protection (PD) ever needed — TX occupies only one bucket at a
time and finishes before camera returns. Camera never encounters a
protected bucket, so no skips occur. Frames complete every 4 ticks
(F0 at tick 4, F1 at tick 8, F2 at tick 12, ...). 0% drop rate.

---

# 🔹 Bucket States

Except at startup:

* If a bucket is not WRITING → it is FULL.
* Only one bucket can be WRITING at a time.
* During TX, a bucket is temporarily protected if camera already finished writing.

There is no persistent EMPTY state during normal operation.

---

# 🔹 Core Goal

* Camera never waits.
* Ethernet never waits.
* We always transmit one Upper and one Lower belonging to the same logical frame.
* Frame drops are allowed.



