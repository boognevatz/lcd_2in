
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

**Protection lifecycle of a TX pair:**

A bucket earns protection only through one of three paths:

1. **Valid unsent data at pair selection:** When TX selects a pair, a bucket that already holds valid, unsent camera data and camera is not currently writing it becomes **TXP** immediately. A bucket containing garbage or stale (already-sent) data does not earn protection — camera may freely overwrite it.

2. **Co-write completion (TX REC → TXP):** When TX and camera are simultaneously on the same bucket (TX REC), the bucket is not protected. Once camera completes its write while TX is still sending, the bucket transitions to **TXP**. Camera must skip it until TX finishes.

3. **Queued partner completion (REC → PD):** When the pair's second bucket is being written by camera while TX sends the first, it is not yet protected. Once camera completes its write while TX is still sending the first bucket, the second bucket transitions to **PD**. Camera must skip it until TX sends and finishes it.

Protection ends when TX finishes sending the bucket.

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

**Order within the new pair — exactly two cases exist:**

Because neither camera nor TX ever idles or waits, it is impossible for a
complete next frame (both Upper and Lower) to already sit in the two other
buckets when TX finishes a pair. The camera is always mid-write or has just
finished one half, so only two situations can occur:

**Case 1 — Camera is currently writing an Upper half-frame.**
The bucket being written contains the Upper half (in progress or just
finished). The other bucket holds stale/invalid data (its previous content
was overwritten or belongs to an older frame whose Upper half was lost).
There is no valid matching pair available. TX selects the bucket with the
Upper half as the first bucket of the new pair, and the other bucket as
the second (it will receive the Lower half by the time TX gets to it, or
TX sends whatever is there). This case occurs when TX is as fast as or
faster than the camera 

**Case 2 — Camera is currently writing a Lower half-frame.**
The bucket being written contains the Lower half (in progress or just
finished). The other bucket already holds the **same frame's Upper half**
— a valid, complete Upper half-frame. TX selects the Upper-half bucket
first, Lower-half bucket second. This produces a matched pair from the
same frame. This case occurs when camera is faster than TX 

**In both cases the order is unambiguous:** whichever bucket holds (or is
receiving) the Upper half goes first; the other goes second. There is
never a situation where both buckets hold unrelated Upper halves, or
where the assignment is unclear.

These rules are verified in Scenarios 1, 1b, and 2.

---

## Step-by-Step Scenario Walkthroughs

The following traces show the state of every bucket at each camera DMA write,
illustrating how the system behaves at different relative speeds.

**Notation:**

* TX REC = TX is actively sending this bucket and camera is simultaneously
  writing to it. The bucket is unprotected — camera is assumed to always be
  ahead, so no protection is granted yet.

* TXP = TX is actively sending this bucket and camera has already finished
  writing it. The bucket is protected — camera must skip it.

* PD = TX is not yet sending this bucket, but it is the queued partner of the
  currently-sending bucket. Camera has already finished writing it. 
  Protected — camera must skip it.

* REC = Camera is actively writing to this bucket. TX is not sending it.

* TX = TX is actively sending this bucket. Camera is not writing to it and
  has not recently finished writing it (bucket holds stale or garbage data). No
  protection is relevant.

* - = Bucket is not being written by camera and not being sent by TX. Holds
  stale or previously written data but is freely overwritable.


---

### Scenario 1 — Camera 2× faster than TX

Camera: 1 tick per half-frame. 
TX: 2 ticks per half-frame. Both start at tick 0.

| Tick | Cam          |  TX   | A      | B      | C      | inA | inB | inC |
|------|--------------|-------|--------|--------|--------|-----|-----|-----|
|    0 | F0U->A 1/1   | 'A+b  | TX REC | -      | -      | F0U |  -  |  -  |
|    1 | F0L->B 1/1   | 'A+b  | TXP    | REC    | -      | F0U | F0L |  -  |
|    2 | F1U->C 1/1   | 'a+B  | -      | TXP    | REC    | F0U | F0L | F1U |
|    3 | F1L->A 1/1   | 'a+B  | REC    | TXP    | -      | F1L | F0L | F1U |
|    4 | F2U->B 1/1   | "C+a  | PD     | REC    | TXP    | F1L | F2U | F1U |
|    5 | F2L->B 1/1   | "C+a  | PD     | REC    | TXP    | F1L | F2L | F1U |
|    6 | F3U->C 1/1   | "c+A  | TXP    | -      | REC    | F1L | F2L | F3U |
|    7 | F3L->B 1/1   | "c+A  | TXP    | REC    | -      | F1L | F3L | F3U |
|    8 | F4U->A 1/1   | 'C+b  | REC    | PD     | TXP    | F4U | F3L | F3U |
|    9 | F4L->A 1/1   | 'C+b  | REC    | PD     | TXP    | F4L | F3L | F3U |
|   10 | F5U->C 1/1   | 'c+B  | -      | TXP    | REC    | F4L | F3L | F5U |
|   11 | F5L->A 1/1   | 'c+B  | REC    | TXP    | -      | F5L | F3L | F5U |
|   12 | F6U->B 1/1   | "C+a  | PD     | REC    | TXP    | F5L | F6U | F5U |

**Result:** Frame 0 sent immediately at startup. 
Sent: F0U + F0L, F1U + F1L, F3U + F3L, F5U+(this is already tick 12)

---

### Scenario 1b — Camera 3× faster than TX

Camera: 1 tick per half-frame. 
TX: 3 ticks per half-frame. Both start at tick 0.

| Tick | Cam          |  TX   | A      | B      | C      | inA | inB | inC |
|------|--------------|-------|--------|--------|--------|-----|-----|-----|
|    0 | F0U->A 1/1   | 'A+b  | TX REC | -      | -      | F0U |  -  |  -  |
|    1 | F0L->B 1/1   | 'A+b  | TXP    | REC    | -      | F0U | F0L |  -  |
|    2 | F1U->C 1/1   | 'A+b  | TXP    | PD     | REC    | F0U | F0L | F1U |
|    3 | F1L->A 1/1   | 'a+B  | REC    | TXP    | -      | F1L | F0L | F1U |
|    4 | F2U->C 1/1   | 'a+B  | -      | TXP    | REC    | F1L | F0L | F2U |
|    5 | F2L->A 1/1   | 'a+B  | REC    | TXP    | -      | F2L | F0L | F2U |
|    6 | F3U->B 1/1   | "C+a  | PD     | REC    | TXP    | F2L | F3U | F2U |
|    7 | F3L->B 1/1   | "C+a  | PD     | REC    | TXP    | F2L | F3L | F2U |
|    8 | F4U->B 1/1   | "C+a  | PD     | REC    | TXP    | F2L | F4U | F2U |
|    9 | F4L->C 1/1   | "c+A  | TXP    | -      | REC    | F2L | F4U | F4L |
|   10 | F5U->B 1/1   | "c+A  | TXP    | REC    | -      | F2L | F5U | F4L |
|   11 | F5L->C 1/1   | "c+A  | TXP    | -      | REC    | F2L | F5U | F5L |
|   12 | F6U->A 1/1   | 'B+c  | REC    | TXP    | PD     | F6U | F5U | F5L |
|   13 | F6L->A 1/1   | 'B+c  | REC    | TXP    | PD     | F6L | F5U | F5L |
|   14 | F7U->A 1/1   | 'B+c  | REC    | TXP    | PD     | F7U | F5U | F5L |
|   15 | F7L->B 1/1   | 'b+C  | -      | REC    | TXP    | F7U | F7L | F5L |
|   16 | F8U->A 1/1   | 'b+C  | REC    | -      | TXP    | F8U | F7L | F5L |
|   17 | F8L->B 1/1   | 'b+C  | -      | REC    | TXP    | F8U | F8L | F5L |
|   18 | F9U->C 1/1   | "A+b  | TXP    | PD     | REC    | F8U | F8L | F9U |

**Result:** Frame 0 sent at startup (F0U+F0L). Then Frame 2, Frame 5, Frame 8, Frame 11, ... Steady-state: every 3rd frame sent, 67% drop rate. The 6-tick TX cycle (3 ticks upper + 3 ticks lower) repeats with the pair rotating C+A -> B+C -> A+B -> C+A -> ...

---

### Scenario 2 — Camera same speed as TX

Camera: 1 tick per half-frame.
TX: 1 tick per half-frame. Both start at tick 0.

| Tick | Cam          |  TX   | A      | B      | C      | inA | inB | inC |
|------|--------------|-------|--------|--------|--------|-----|-----|-----|
|    0 | F0U->A 1/1   | 'A+b  | TX REC | -      | -      | F0U |  -  |  -  |
|    1 | F0L->B 1/1   | 'a+B  | -      | TX REC | -      | F0U | F0L |  -  |
|    2 | F1U->C 1/1   | "C+a  | -      | -      | TX REC | F0U | F0L | F1U |
|    3 | F1L->A 1/1   | "c+A  | TX REC | -      | -      | F1L | F0L | F1U |
|    4 | F2U->B 1/1   | 'B+c  | -      | TX REC | -      | F1L | F2U | F1U |
|    5 | F2L->C 1/1   | 'b+C  | -      | -      | TX REC | F1L | F2U | F2L |
|    6 | F3U->A 1/1   | "A+b  | TX REC | -      | -      | F3U | F2U | F2L |
|    7 | F3L->B 1/1   | "a+B  | -      | TX REC | -      | F3U | F3L | F2L |
|    8 | F4U->C 1/1   | 'C+a  | -      | -      | TX REC | F3U | F3L | F4U |

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

TX is faster than camera. TX never idles — it always sends the next pair
immediately, even if buckets contain garbage or stale data from previous frames.

| Tick | Cam          |  TX  | A      | B      | C      | inA | inB | inC |
|------|--------------|------|--------|--------|--------|-----|-----|-----|
|    0 | F0U->A 1/2   | 'A+b | TX REC | -      | -      | F0U |  -  |  -  |
|    1 | F0U->A 2/2   | 'a+B | REC    | TX     | -      | F0U |  -  |  -  |
|    2 | F0L->B 1/2   | "A+c | TXP    | REC    | -      | F0U | F0L |  -  |
|    3 | F0L->B 2/2   | "a+C | -      | REC    | TX     | F0U | F0L |  -  |
|    4 | F1U->C 1/2   | 'A+b | TX     | PD     | REC    | F0U | F0L | F1U |
|    5 | F1U->C 2/2   | 'a+B | -      | TXP    | REC    | F0U | F0L | F1U |
|    6 | F1L->A 1/2   | "C+a | REC    | -      | TXP    | F1L | F0L | F1U |
|    7 | F1L->A 2/2   | "c+A | TX REC | -      | -      | F1L | F0L | F1U |
|    8 | F2U->B 1/2   | 'B+c | -      | TX REC | -      | F1L | F2U | F1U |
|    9 | F2U->B 2/2   | 'b+C | -      | REC    | TX     | F1L | F2U | F1U |
|   10 | F2L->C 1/2   | "B+a | PD     | TXP    | REC    | F1L | F2U | F2L |
|   11 | F2L->C 2/2   | "b+A | TXP    | -      | REC    | F1L | F2U | F2L |
|   12 | F3U->A 1/2   | 'B+c | REC    | TX     | PD     | F3U | F2U | F2L |
|   13 | F3U->A 2/2   | 'b+C | REC    | -      | TXP    | F3U | F2U | F2L |



**Result:**
TX never idles — it always sends immediately, even at startup when buckets
contain garbage.

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
* TX always sends a matched Upper+Lower pair from the same frame when
  possible. This is best-effort — TX never idles, so when a matched pair
  is unavailable, TX sends whatever is in the selected buckets. Broken or
  mismatched half-frames are an accepted outcome by design.
* Frame drops are allowed.
* TX always selects the two buckets that are not the bucket just finished.
* The selection of buckets is deterministic and cannot be changed.

* TX only controls the order of the two selected buckets.

* TX orders the buckets so that an Upper half-frame is transmitted before a Lower half-frame whenever possible.

* If the two selected buckets contain a matched Upper+Lower pair from the same frame, TX sends them in that order.

* If no matched pair exists, TX still chooses the order consistent with the camera write direction (Upper first).

* TX never idles — transmission is continuous. When a matched pair is not available (e.g., at startup or when TX outpaces camera), TX sends whatever is in the selected buckets, which may be garbage, stale, or mismatched data. The receiver is responsible for discarding incomplete or invalid frames.


