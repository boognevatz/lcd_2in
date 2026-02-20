
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

---

## Step-by-Step Scenario Walkthroughs

The following traces show the state of every bucket at each camera DMA write,
illustrating how the system behaves at different relative speeds.

**Notation:**
* TX  = TX is actively sending this bucket (the camera may write it simultaneously)
* TX REC = TX is sending this bucket AND camera is writing to it simultaneously (unprotected — we          [ ] Write Scenario 3 to file after
     assume camera is always ahead)                                                                                 user review
* TXP = TX is actively sending this bucket, and it is protected (camera already finished writing this
     frame's data)                                                                                              Modified Files
* PD = bucket is protected but NOT being sent — it's the queued partner of the currently-sending bucket    three_bucket_system.md       +32 -112
      (e.g., the Lower half while TX sends the Upper half)
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



