
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
* TX  = TX is actively sending this bucket (the camera may write it simultanously)
* TXP = TX is actively sending this bucket, and it is protected, camera can not write here
* PD  = protected for TX (queued, not yet being sent)
* REC = camera is writing to this bucket this tick
* FREE= bucket is free for writing (empty or stale (frame droped) data)
* F2U = Frame 2 Upper
* F2L = Frame 2 Lower

---

### Scenario 1 — Camera 2× faster than TX

Camera: 1 tick per half-frame. TX: 2 ticks per half-frame. Both start at tick 0.

| Tick | Camera writes |     A |     B |     C |                                           TX action |
|------|---------------|-------|-------|-------|-----------------------------------------------------|
|    0 |      F0U -> A | TX REC|  FREE |  FREE | TX starts sending A, which contains partial F0U     |
|    1 |      F0L -> B |   TXP |   REC |  FREE | TX is still sending A, it is protected now          |
|    2 |      F1U -> C |  FREE |   TXP |   REC | TX frees A. Starts sending B                        |
|    3 |      F1L -> A |   REC |   TXP |   F1U | TX finishes B, Camera finishes A                    |
|    4 |      F2U -> B | F1L PD|   REC |   TXP | TX starts C+A (protected), camera can use B and B only|
|    5 |      F2L -> B | F1L PD|   REC |   TXP | TX continus sending C. Camera overwrites B, since A is protected.|
|    6 |      F3U -> C |   TXP |   F2L |   REC | TX frees C, starts sending A. Camera immediatly starts using C.       |
|    7 |      F3L -> B |   TXP |   REC |   F3U | TX finishes A,                                      |
|    8 |      F4U -> A |   REC | F3L PD| F3U TX| TX starts to send C+B                               |
|    9 |      F4L -> A |   REC | F3L PD| F3U TX| Only A free. Overwrite A. Frame 4 dropped.          |
|   10 |      F5U -> C |   F4L | F3L TX|   REC | TX frees C. Camera immediatly starts using it.      |
|   11 |      F5L -> A |   REC | F3L TX|   F5U | F5 is ready, camera: C->A (free).                   |
|   12 |      F6U -> B | F5L PD|   REC | F5U TX| TX frees B.                                         |

**Result:** Frame 0 sent at startup. Sent: F0U + F0L, F1U + F1L, 

---

### Scenario 2 — Camera same speed as TX

Camera: 1 tick per half-frame. TX: 1 tick per half-frame.

| Tick | Camera writes |     A |     B |     C | TX action |
|------|---------------|-------|-------|-------|-----------|
|    0 |       A ← F0U |     ← |     — |     — |      idle |
|    1 |       B ← F0L |   F0U |     ← |     — |      idle |
|    2 |       C ← F1U |F0U [S]|F0L [P]|     ← | TX picks **(A,B)**. Sends A (t=2→3). |
|    3 |       A ← F1L |     ← |F0L [S]|   F1U | TX frees A. Sends B (t=3→4). Camera: C→A (free). |
|    4 |       B ← F2U |F1L [P]|     ← |F1U [S]| TX frees B. **Picks (C,A).** C=F1U, A=F1L. **Frame 1 complete!** Sends C (t=4→5). Camera: A`[P]`→skip→B (free). |
|    5 |       C ← F2L |F1L [S]|   F2U |     ← | TX frees C. Sends A (t=5→6). Camera: B→C (free). |
|    6 |       A ← F3U |     ← |F2U [S]|F2L [P]| TX frees A. Picks (B,C). B=F2U, C=F2L. **Frame 2 complete!** Sends B (t=6→7). Camera: C`[P]`→skip→A (free). |
|    7 |       B ← F3L |   F3U |     ← |F2L [S]| TX frees B. Sends C (t=7→8). Camera: A→B (free). |
|    8 |       C ← F4U |F3U [S]|F3L [P]|     ← | TX frees C. **Picks (A,B).** A=F3U, B=F3L. **Frame 3 complete!** |

**Result:** Frames sent: 0, 1, 2, 3, … **No frames dropped.** Every frame is transmitted.

---

### Summary

| Camera vs TX speed | Frames dropped  |            Behavior |
|--------------------|-----------------|---------------------|
|         Same speed |            None |    Every frame sent |
|  Camera 2× faster |Every other frame |       50% drop rate |
|  Camera 3× faster | 2 out of every 3 |      ~67% drop rate |

In all cases the system is **self-correcting**. No state accumulates — each TX cycle is independent.

**Design assumption:** When the camera and TX are simultaneously accessing the **same** bucket (camera writing, TX reading), we assume the camera is always ahead of TX within that bucket. With only 3 buckets, this is a necessary design constraint. If violated momentarily (e.g., a few pixels at the very start of simultaneous access), the system self-corrects as the camera outpaces TX.

---

# 🔹 Bucket States

Except at startup:

* If a bucket is not WRITING → it is FULL.
* Only one bucket can be WRITING at a time.
* During TX, a bucket is temporarily protected.

There is no persistent EMPTY state during normal operation.

---

# 🔹 Core Goal

* Camera never waits.
* Ethernet never waits.
* We always transmit one Upper and one Lower belonging to the same logical frame.
* Frame drops are allowed.

---

# 🔹 Transmission Is Half-Driven

We do **not** wait for a fully completed frame.

When transmission must decide the next pair, there are only two real situations:

---

# Case I — A is WRITING

* A = currently being written (Upper of new frame)
* C = contains a Lower half (which belong to previous/dropped frame, 
so in the moment of sending it is trash)

We select pair **A + C**.

Meaning:

* We start transmitting C or A depending on order policy.
* By the time transmission needs the other half, the camera will have progressed.
* The sender never catches up to the camera.
* Frame alignment self-corrects because the camera is faster.

This is speculative pairing based on pipeline timing.

---

# Case II — C is WRITING

* A = contains Upper half
* C = currently being written Lower half of the same frame

We select pair **A + C**.

Here the frame is naturally aligned:

* Upper already exists.
* Lower is in progress.
* By the time we transmit the second half, writing is complete.

# We need to generalize the above concept

The above two cases, A is writing and C is writing, it is the case 
after we initially finished transmitting A+B full frame (at startup).
But any given time, once a lower halfframe is transmitted and we 
need to transmit a new frame, there is always the other two bucket where
the transmission will happen. And depending on the camera speed 
it is either the upper halfframe is being written by the camera (so we selecting 
that frame plus the other bucket), or the lower frame is being written by 
the camera (in that case the other bucket where we start sending, then the 
lower halfframe will be.)

The key point we determine the two bucket instantly when we finished 
sending.

---

# 🔹 Early Freeing Rule

When sending a pair (X, Y):

1. Send first bucket.
2. Immediately mark it FREE (camera may reuse it).
3. Send second bucket.
4. Mark it FREE after TX complete.

Once we sent the upper half frame, we sending the lower half frame.
Only the lower half frame needs to be protected, so not rewqrite it by a next 
frame. But it may be the case that the lower half frame is being still 
written by the camera.


