
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

### Scenario 2 — Camera 3× faster than TX

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

### Scenario 3 — Camera same speed as TX

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

### Scenario 4 — Camera 2× slower than TX

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


### Scenario 5 — Camera 5 ticks, TX 4 ticks per half-frame (TX slightly faster)


| Tick | Cam          | TX    | A      | B      | C      | inA  | inB  | inC  |
|------|--------------|-------|--------|--------|--------|------|------|------|
|    0 | F0U->A 1/5   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    1 | F0U->A 2/5   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    2 | F0U->A 3/5   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    3 | F0U->A 4/5   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    4 | F0U->A 5/5   | 'a+B  | REC    | TX     | -      | -    | -    | -    |
|    5 | F0L->B 1/5   | 'a+B  | -      | TX REC | -      | F0U  | -    | -    |
|    6 | F0L->B 2/5   | 'a+B  | -      | TX REC | -      | F0U  | -    | -    |
|    7 | F0L->B 3/5   | 'a+B  | -      | TX REC | -      | F0U  | -    | -    |
|    8 | F0L->B 4/5   | "A+c  | TXP    | REC    | -      | F0U  | -    | -    |
|    9 | F0L->B 5/5   | "A+c  | TXP    | REC    | -      | F0U  | -    | -    |
|   10 | F1U->C 1/5   | "A+c  | TXP    | -      | REC    | F0U  | F0L  | -    |
|   11 | F1U->C 2/5   | "A+c  | TXP    | -      | REC    | F0U  | F0L  | -    |
|   12 | F1U->C 3/5   | "a+C  | -      | -      | TX REC | F0U  | F0L  | -    |
|   13 | F1U->C 4/5   | "a+C  | -      | -      | TX REC | F0U  | F0L  | -    |
|   14 | F1U->C 5/5   | "a+C  | -      | -      | TX REC | F0U  | F0L  | -    |
|   15 | F1L->A 1/5   | "a+C  | REC    | -      | TXP    | F0U  | F0L  | F1U  |
|   16 | F1L->A 2/5   | 'A+b  | TX REC | PD     | -      | F0U  | F0L  | F1U  |
|   17 | F1L->A 3/5   | 'A+b  | TX REC | PD     | -      | F0U  | F0L  | F1U  |
|   18 | F1L->A 4/5   | 'A+b  | TX REC | PD     | -      | F0U  | F0L  | F1U  |
|   19 | F1L->A 5/5   | 'A+b  | TX REC | PD     | -      | F0U  | F0L  | F1U  |
|   20 | F2U->C 1/5   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   21 | F2U->C 2/5   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   22 | F2U->C 3/5   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   23 | F2U->C 4/5   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   24 | F2U->C 5/5   | 'C+a  | -      | -      | TX REC | F1L  | F0L  | F1U  |
|   25 | F2L->A 1/5   | 'C+a  | REC    | -      | TXP    | F1L  | F0L  | F2U  |
|   26 | F2L->A 2/5   | 'C+a  | REC    | -      | TXP    | F1L  | F0L  | F2U  |
|   27 | F2L->A 3/5   | 'C+a  | REC    | -      | TXP    | F1L  | F0L  | F2U  |
|   28 | F2L->A 4/5   | 'c+A  | TX REC | -      | -      | F1L  | F0L  | F2U  |
|   29 | F2L->A 5/5   | 'c+A  | TX REC | -      | -      | F1L  | F0L  | F2U  |
|   30 | F3U->B 1/5   | 'c+A  | TXP    | REC    | -      | F2L  | F0L  | F2U  |
|   31 | F3U->B 2/5   | 'c+A  | TXP    | REC    | -      | F2L  | F0L  | F2U  |
|   32 | F3U->B 3/5   | 'B+c  | -      | TX REC | -      | F2L  | F0L  | F2U  |
|   33 | F3U->B 4/5   | 'B+c  | -      | TX REC | -      | F2L  | F0L  | F2U  |
|   34 | F3U->B 5/5   | 'B+c  | -      | TX REC | -      | F2L  | F0L  | F2U  |
|   35 | F3L->C 1/5   | 'B+c  | -      | TXP    | REC    | F2L  | F3U  | F2U  |
|   36 | F3L->C 2/5   | 'b+C  | -      | -      | TX REC | F2L  | F3U  | F2U  |
|   37 | F3L->C 3/5   | 'b+C  | -      | -      | TX REC | F2L  | F3U  | F2U  |
|   38 | F3L->C 4/5   | 'b+C  | -      | -      | TX REC | F2L  | F3U  | F2U  |
|   39 | F3L->C 5/5   | 'b+C  | -      | -      | TX REC | F2L  | F3U  | F2U  |
|   40 | F4U->A 1/5   | 'A+b  | TX REC | -      | -      | F2L  | F3U  | F3L  |
|   41 | F4U->A 2/5   | 'A+b  | TX REC | -      | -      | F2L  | F3U  | F3L  |
|   42 | F4U->A 3/5   | 'A+b  | TX REC | -      | -      | F2L  | F3U  | F3L  |
|   43 | F4U->A 4/5   | 'A+b  | TX REC | -      | -      | F2L  | F3U  | F3L  |


---

### Scenario 6 — Camera 3 ticks, TX 7 ticks per half-frame (TX ~2.33× slower than camera)

| Tick | Cam          | TX    | A      | B      | C      | inA  | inB  | inC  |
|------|--------------|-------|--------|--------|--------|------|------|------|
|    0 | F0U->A 1/3   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    1 | F0U->A 2/3   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    2 | F0U->A 3/3   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    3 | F0L->B 1/3   | "A+b  | TXP    | REC    | -      | F0U  | -    | -    |
|    4 | F0L->B 2/3   | "A+b  | TXP    | REC    | -      | F0U  | -    | -    |
|    5 | F0L->B 3/3   | "A+b  | TXP    | REC    | -      | F0U  | -    | -    |
|    6 | F1U->C 1/3   | "A+b  | TXP    | PD     | REC    | F0U  | F0L  | -    |
|    7 | F1U->C 2/3   | "a+B  | -      | TXP    | REC    | F0U  | F0L  | -    |
|    8 | F1U->C 3/3   | "a+B  | -      | TXP    | REC    | F0U  | F0L  | -    |
|    9 | F1L->A 1/3   | "a+B  | REC    | TXP    | -      | F0U  | F0L  | F1U  |
|   10 | F1L->A 2/3   | "a+B  | REC    | TXP    | -      | F0U  | F0L  | F1U  |
|   11 | F1L->A 3/3   | "a+B  | REC    | TXP    | -      | F0U  | F0L  | F1U  |
|   12 | F2U->C 1/3   | "a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   13 | F2U->C 2/3   | "a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   14 | F2U->C 3/3   | 'C+a  | PD     | -      | TX REC | F1L  | F0L  | F1U  |
|   15 | F2L->B 1/3   | "C+a  | PD     | REC    | TXP    | F1L  | F0L  | F2U  |
|   16 | F2L->B 2/3   | "C+a  | PD     | REC    | TXP    | F1L  | F0L  | F2U  |
|   17 | F2L->B 3/3   | "C+a  | PD     | REC    | TXP    | F1L  | F0L  | F2U  |
|   18 | F3U->B 1/3   | "C+a  | PD     | REC    | TXP    | F1L  | F2L  | F2U  |
|   19 | F3U->B 2/3   | "C+a  | PD     | REC    | TXP    | F1L  | F2L  | F2U  |
|   20 | F3U->B 3/3   | "C+a  | PD     | REC    | TXP    | F1L  | F2L  | F2U  |
|   21 | F3L->B 1/3   | "c+A  | TXP    | REC    | -      | F1L  | F3U  | F2U  |
|   22 | F3L->B 2/3   | "c+A  | TXP    | REC    | -      | F1L  | F3U  | F2U  |
|   23 | F3L->B 3/3   | "c+A  | TXP    | REC    | -      | F1L  | F3U  | F2U  |
|   24 | F4U->C 1/3   | "c+A  | TXP    | -      | REC    | F1L  | F3L  | F2U  |
|   25 | F4U->C 2/3   | "c+A  | TXP    | -      | REC    | F1L  | F3L  | F2U  |
|   26 | F4U->C 3/3   | "c+A  | TXP    | -      | REC    | F1L  | F3L  | F4U  |
|   27 | F4L->B 1/3   | "c+A  | TXP    | REC    | -      | F1L  | F3L  | F4U  |
|   28 | F4L->B 2/3   | "C+b  | -      | REC    | TXP    | F1L  | F3L  | F4U  |
|   29 | F4L->B 3/3   | "C+b  | -      | REC    | TXP    | F1L  | F3L  | F4U  |
|   30 | F5U->A 1/3   | "C+b  | REC    | PD     | TXP    | F1L  | F4L  | F4U  |
|   31 | F5U->A 2/3   | "C+b  | REC    | PD     | TXP    | F1L  | F4L  | F4U  |
|   32 | F5U->A 3/3   | "C+b  | REC    | PD     | TXP    | F1L  | F4L  | F4U  |
|   33 | F5L->A 1/3   | "C+b  | REC    | PD     | TXP    | F5U  | F4L  | F4U  |
|   34 | F5L->A 2/3   | "C+b  | REC    | PD     | TXP    | F5U  | F4L  | F4U  |
|   35 | F5L->A 3/3   | "c+B  | REC    | TXP    | -      | F5U  | F4L  | F4U  |
|   36 | F6U->C 1/3   | "c+B  | -      | TXP    | REC    | F5L  | F4L  | F4U  |
|   37 | F6U->C 2/3   | "c+B  | -      | TXP    | REC    | F5L  | F4L  | F4U  |
|   38 | F6U->C 3/3   | "c+B  | -      | TXP    | REC    | F5L  | F4L  | F6U  |
|   39 | F6L->A 1/3   | "c+B  | REC    | TXP    | -      | F5L  | F4L  | F6U  |
|   40 | F6L->A 2/3   | "c+B  | REC    | TXP    | -      | F5L  | F4L  | F6U  |
|   41 | F6L->A 3/3   | "c+B  | REC    | TXP    | -      | F5L  | F4L  | F6U  |
|   42 | F7U->C 1/3   | 'C+a  | PD     | -      | TX REC | F6L  | F4L  | F6U  |
|   43 | F7U->C 2/3   | 'C+a  | PD     | -      | TX REC | F6L  | F4L  | F6U  |
|   44 | F7U->C 3/3   | 'C+a  | PD     | -      | TX REC | F6L  | F4L  | F7U  |
|   45 | F7L->B 1/3   | "C+a  | PD     | REC    | TXP    | F6L  | F4L  | F7U  |
|   46 | F7L->B 2/3   | "C+a  | PD     | REC    | TXP    | F6L  | F4L  | F7U  |
|   47 | F7L->B 3/3   | "C+a  | PD     | REC    | TXP    | F6L  | F4L  | F7U  |
|   48 | F8U->B 1/3   | "C+a  | PD     | REC    | TXP    | F6L  | F7L  | F7U  |
|   49 | F8U->B 2/3   | "c+A  | TXP    | REC    | -      | F6L  | F7L  | F7U  |
|   50 | F8U->B 3/3   | "c+A  | TXP    | REC    | -      | F6L  | F7L  | F8U  |
|   51 | F8L->C 1/3   | "c+A  | TXP    | -      | REC    | F6L  | F8U  | F7U  |
|   52 | F8L->C 2/3   | "c+A  | TXP    | -      | REC    | F6L  | F8U  | F7U  |
|   53 | F8L->C 3/3   | "c+A  | TXP    | -      | REC    | F6L  | F8U  | F8L  |
|   54 | F9U->B 1/3   | "c+A  | TXP    | REC    | -      | F6L  | F8U  | F8L  |
|   55 | F9U->B 2/3   | "c+A  | TXP    | REC    | -      | F6L  | F8U  | F8L  |
|   56 | F9U->B 3/3   | 'B+c  | -      | TX REC | PD     | F6L  | F8U  | F8L  |
|   57 | F9L->A 1/3   | "B+c  | REC    | TXP    | PD     | F6L  | F9U  | F8L  |
|   58 | F9L->A 2/3   | "B+c  | REC    | TXP    | PD     | F6L  | F9U  | F8L  |
|   59 | F9L->A 3/3   | "B+c  | REC    | TXP    | PD     | F6L  | F9U  | F8L  |
|   60 | F10U->A 1/3  | "B+c  | REC    | TXP    | PD     | F9L  | F9U  | F8L  |
|   61 | F10U->A 2/3  | "B+c  | REC    | TXP    | PD     | F9L  | F9U  | F8L  |
|   62 | F10U->A 3/3  | "B+c  | REC    | TXP    | PD     | F9L  | F9U  | F8L  |
|   63 | F10L->A 1/3  | "b+C  | REC    | -      | TXP    | F10U | F9U  | F8L  |
|   64 | F10L->A 2/3  | "b+C  | REC    | -      | TXP    | F10U | F9U  | F8L  |
|   65 | F10L->A 3/3  | "b+C  | REC    | -      | TXP    | F10U | F9U  | F8L  |
|   66 | F11U->B 1/3  | "b+C  | -      | REC    | TXP    | F10L | F9U  | F8L  |
|   67 | F11U->B 2/3  | "b+C  | -      | REC    | TXP    | F10L | F9U  | F8L  |
|   68 | F11U->B 3/3  | "b+C  | -      | REC    | TXP    | F10L | F9U  | F8L  |
|   69 | F11L->A 1/3  | "b+C  | REC    | -      | TXP    | F10L | F11U | F8L  |
|   70 | F11L->A 2/3  | "B+a  | REC    | TXP    | -      | F10L | F11U | F8L  |
|   71 | F11L->A 3/3  | "B+a  | REC    | TXP    | -      | F10L | F11U | F8L  |
|   72 | F12U->C 1/3  | "B+a  | PD     | TXP    | REC    | F11L | F11U | F8L  |
|   73 | F12U->C 2/3  | "B+a  | PD     | TXP    | REC    | F11L | F11U | F8L  |
|   74 | F12U->C 3/3  | "B+a  | PD     | TXP    | REC    | F11L | F11U | F12U |
|   75 | F12L->C 1/3  | "B+a  | PD     | TXP    | REC    | F11L | F11U | F12U |
|   76 | F12L->C 2/3  | "B+a  | PD     | TXP    | REC    | F11L | F11U | F12U |
|   77 | F12L->C 3/3  | "b+A  | TXP    | -      | REC    | F11L | F11U | F12U |
|   78 | F13U->B 1/3  | "b+A  | TXP    | REC    | -      | F11L | F11U | F12L |
|   79 | F13U->B 2/3  | "b+A  | TXP    | REC    | -      | F11L | F11U | F12L |
|   80 | F13U->B 3/3  | "b+A  | TXP    | REC    | -      | F11L | F13U | F12L |
|   81 | F13L->C 1/3  | "b+A  | TXP    | -      | REC    | F11L | F13U | F12L |
|   82 | F13L->C 2/3  | "b+A  | TXP    | -      | REC    | F11L | F13U | F12L |
|   83 | F13L->C 3/3  | "b+A  | TXP    | -      | REC    | F11L | F13U | F13L |
|   84 | F14U->B 1/3  | 'B+c  | -      | TX REC | PD     | F11L | F13U | F13L |
|   85 | F14U->B 2/3  | 'B+c  | -      | TX REC | PD     | F11L | F13U | F13L |
|   86 | F14U->B 3/3  | 'B+c  | -      | TX REC | PD     | F11L | F13U | F13L |
|   87 | F14L->A 1/3  | "B+c  | REC    | TXP    | PD     | F11L | F14U | F13L |
|   88 | F14L->A 2/3  | "B+c  | REC    | TXP    | PD     | F11L | F14U | F13L |
|   89 | F14L->A 3/3  | "B+c  | REC    | TXP    | PD     | F11L | F14U | F13L |

---

**Key observations — this is the richest scenario yet:**

**Tick 5→6:** Camera finishes F0L→B while TX still on A (TXP). B was `tx_partner` → instantly becomes `PD`. This is the earliest PD appearance across all scenarios — camera is so fast it fills the partner slot while TX is still on the first bucket.

**Ticks 14→15: `'C+a` — Case 1 moment.** TX finishes B (second half). Candidates: {A, C}. Camera writing F2U (Upper) into C → Case 1. C is taken immediately as TX's upper slot. But cam_wrote_valid[A] was reset (TX sent it) → A is **not** PD. A just sits free. Then camera finishes C at tick 14 simultaneously with TX arriving → `TX REC`.

**Ticks 30–34: A=REC, B=PD, C=TXP simultaneously** — all three buckets in a non-idle, non-free state at the same time. Camera is writing A (which it keeps overwriting since it can't go to B or C), B is queued protected, C is being sent. This is the "camera trapped on one bucket" scenario from the rules, demonstrated live.

**Sent pair results:**

| Completed at tick | Upper  | Lower  | Result         |
|-------------------|--------|--------|----------------|
| 13                | F0U ✓  | F0L ✓  | **Frame 0 ✓**  |
| 27                | F2U ✓  | F1L ✓  | mismatch       |
| 41                | F4U ✓  | F4L ✓  | **Frame 4 ✓**  |
| 55                | F7U ✓  | F6L ✓  | mismatch       |
| 69                | F9U ✓  | F8L ✓  | mismatch       |
| 83                | F11U ✓ | F11L ✓ | **Frame 11 ✓** |

**Result:** Every 14 ticks TX completes a pair. Matched frames appear irregularly — not every 3rd frame as in Scenario 1b, but with an aperiodic pattern driven by the 3:7 ratio. Drop rate is high (~67% of frames lost), but the mismatches are always valid data (both halves complete, just from different frames). The `"` prefix dominates throughout — Case 2 rules this scenario, with camera almost always having a complete upper half ready when TX needs a new pair.

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


