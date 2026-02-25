
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

**Order within the new pair — three cases exist:**

Because neither camera nor TX ever idles or waits, their relative speeds 
determine the state of the two available candidate buckets when TX finishes 
a pair. The core rule always applies: TX selects the bucket holding (or 
receiving) the Upper half first, and the other bucket second.

Depending on the camera's speed relative to TX, three situations can occur:

**Case 1 — The Race (Simultaneous Finish):**
This is the theoretically possible but practically impossible edge case where
camera and TX finish at the exact same microsecond. Camera completes writing
the Lower half of a frame into one candidate bucket at the precise instant TX
finishes sending the previous pair's last bucket. At this moment, both
candidate buckets hold a complete frame (Upper in one, Lower in the other).
TX must immediately lock both buckets for transmission (Upper first, then
Lower) before the camera's next write overwrites one of them. This case is
demonstrated in Scenario 2 (tick 5→6), Scenario 5 (tick 39→40), and
Scenario 6 (ticks 41→42 and 83→84).

**Case 2 — Camera is currently writing a Lower half-frame.**
The camera has finished the Upper half in one candidate bucket and is 
currently writing the Lower half into the other candidate bucket. TX 
selects the complete Upper-half bucket first, and the Lower-half bucket 
second. This seamlessly forms a matched pair from the same frame.

**Case 3 — Camera is currently writing an Upper half-frame.**
The bucket being written contains the Upper half (in progress or just
finished). The other bucket holds stale/invalid data (its previous content
was overwritten or belongs to an older frame whose Upper half was lost).
There is no valid matching pair available. TX selects the bucket with the
Upper half as the first bucket of the new pair, and the other bucket as
the second (it will receive the Lower half by the time TX gets to it, or
TX sends whatever is there). 

**In all cases the order is unambiguous:** whichever bucket holds (or is
receiving) the Upper half goes first; the other goes second. There is
never a situation where both buckets hold unrelated Upper halves, or
where the assignment is unclear.

These rules are verified in Scenarios 1 through 6.

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

**Cam column:** `F0U->A 1/1` means Frame 0 Upper half, writing to bucket A,
progress 1 of 1 ticks. `F2L->B 2/3` means Frame 2 Lower half, writing to
bucket B, tick 2 of 3 total ticks needed to complete the write.

**TX column:** The letter(s) identify which buckets form the current TX pair.
Uppercase = currently being sent. Lowercase = already sent or queued next.
The first letter is the pair's Upper-half bucket, the second is the Lower-half
bucket, separated by `+`. Prefix `'` and `"` alternate between successive
pairs to visually distinguish pair boundaries — `'` marks odd pairs (1st, 3rd,
5th, ...) and `"` marks even pairs (2nd, 4th, 6th, ...). Example: `'A+b`
means pair 1, currently sending A (Upper), B queued next (Lower). `"c+A` means
pair 2, C already sent (Upper), now sending A (Lower).

**inA, inB, inC columns:** Track which half-frame data each bucket currently
holds. Updated when camera finishes writing to that bucket (i.e., on the last
tick of a multi-tick write, or on the single tick of a 1-tick write). `-` means
no valid data has been written yet (startup only).


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
|    0 | F0U->A 1/2   | 'A+b | TX REC | -      | -      |  -  |  -  |  -  |
|    1 | F0U->A 2/2   | 'a+B | REC    | TX     | -      | F0U |  -  |  -  |
|    2 | F0L->B 1/2   | "A+c | TXP    | REC    | -      | F0U |  -  |  -  |
|    3 | F0L->B 2/2   | "a+C | -      | REC    | TX     | F0U | F0L |  -  |
|    4 | F1U->C 1/2   | 'A+b | TX     | PD     | REC    | F0U | F0L |  -  |
|    5 | F1U->C 2/2   | 'a+B | -      | TXP    | REC    | F0U | F0L | F1U |
|    6 | F1L->A 1/2   | "C+a | REC    | -      | TXP    | F0U | F0L | F1U |
|    7 | F1L->A 2/2   | "c+A | TX REC | -      | -      | F1L | F0L | F1U |
|    8 | F2U->B 1/2   | 'B+c | -      | TX REC | -      | F1L | F0L | F1U |
|    9 | F2U->B 2/2   | 'b+C | -      | REC    | TX     | F1L | F2U | F1U |
|   10 | F2L->C 1/2   | "B+a | PD     | TXP    | REC    | F1L | F2U | F1U |
|   11 | F2L->C 2/2   | "b+A | TXP    | -      | REC    | F1L | F2U | F2L |
|   12 | F3U->A 1/2   | 'B+c | REC    | TX     | PD     | F1L | F2U | F2L |
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
|    4 | F0U->A 5/5   | 'a+B  | REC    | TX     | -      | F0U  | -    | -    |
|    5 | F0L->B 1/5   | 'a+B  | -      | TX REC | -      | F0U  | -    | -    |
|    6 | F0L->B 2/5   | 'a+B  | -      | TX REC | -      | F0U  | -    | -    |
|    7 | F0L->B 3/5   | 'a+B  | -      | TX REC | -      | F0U  | -    | -    |
|    8 | F0L->B 4/5   | "A+c  | TXP    | REC    | -      | F0U  | -    | -    |
|    9 | F0L->B 5/5   | "A+c  | TXP    | REC    | -      | F0U  | F0L  | -    |
|   10 | F1U->C 1/5   | "A+c  | TXP    | -      | REC    | F0U  | F0L  | -    |
|   11 | F1U->C 2/5   | "A+c  | TXP    | -      | REC    | F0U  | F0L  | -    |
|   12 | F1U->C 3/5   | "a+C  | -      | -      | TX REC | F0U  | F0L  | -    |
|   13 | F1U->C 4/5   | "a+C  | -      | -      | TX REC | F0U  | F0L  | -    |
|   14 | F1U->C 5/5   | "a+C  | -      | -      | TX REC | F0U  | F0L  | F1U  |
|   15 | F1L->A 1/5   | "a+C  | REC    | -      | TXP    | F0U  | F0L  | F1U  |
|   16 | F1L->A 2/5   | 'A+b  | TX REC | PD     | -      | F0U  | F0L  | F1U  |
|   17 | F1L->A 3/5   | 'A+b  | TX REC | PD     | -      | F0U  | F0L  | F1U  |
|   18 | F1L->A 4/5   | 'A+b  | TX REC | PD     | -      | F0U  | F0L  | F1U  |
|   19 | F1L->A 5/5   | 'A+b  | TX REC | PD     | -      | F1L  | F0L  | F1U  |
|   20 | F2U->C 1/5   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   21 | F2U->C 2/5   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   22 | F2U->C 3/5   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   23 | F2U->C 4/5   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   24 | F2U->C 5/5   | "C+a  | PD     | -      | TX REC | F1L  | F0L  | F2U  |
|   25 | F2L->B 1/5   | "C+a  | PD     | REC    | TXP    | F1L  | F0L  | F2U  |
|   26 | F2L->B 2/5   | "C+a  | PD     | REC    | TXP    | F1L  | F0L  | F2U  |
|   27 | F2L->B 3/5   | "C+a  | PD     | REC    | TXP    | F1L  | F0L  | F2U  |
|   28 | F2L->B 4/5   | "c+A  | TXP    | REC    | -      | F1L  | F0L  | F2U  |
|   29 | F2L->B 5/5   | "c+A  | TXP    | REC    | -      | F1L  | F2L  | F2U  |
|   30 | F3U->C 1/5   | "c+A  | TXP    | -      | REC    | F1L  | F2L  | F2U  |
|   31 | F3U->C 2/5   | "c+A  | TXP    | -      | REC    | F1L  | F2L  | F2U  |
|   32 | F3U->C 3/5   | 'C+b  | -      | PD     | TX REC | F1L  | F2L  | F2U  |
|   33 | F3U->C 4/5   | 'C+b  | -      | PD     | TX REC | F1L  | F2L  | F2U  |
|   34 | F3U->C 5/5   | 'C+b  | -      | PD     | TX REC | F1L  | F2L  | F3U  |
|   35 | F3L->A 1/5   | 'C+b  | REC    | PD     | TXP    | F1L  | F2L  | F3U  |
|   36 | F3L->A 2/5   | 'c+B  | REC    | TXP    | -      | F1L  | F2L  | F3U  |
|   37 | F3L->A 3/5   | 'c+B  | REC    | TXP    | -      | F1L  | F2L  | F3U  |
|   38 | F3L->A 4/5   | 'c+B  | REC    | TXP    | -      | F1L  | F2L  | F3U  |
|   39 | F3L->A 5/5   | 'c+B  | REC    | TXP    | -      | F3L  | F2L  | F3U  |
|   40 | F4U->B 1/5   | "C+a  | PD     | REC    | TXP    | F3L  | F2L  | F3U  |
|   41 | F4U->B 2/5   | "C+a  | PD     | REC    | TXP    | F3L  | F2L  | F3U  |
|   42 | F4U->B 3/5   | "C+a  | PD     | REC    | TXP    | F3L  | F2L  | F3U  |
|   43 | F4U->B 4/5   | "C+a  | PD     | REC    | TXP    | F3L  | F2L  | F3U  |

**Result:**
TX is slightly faster than camera (4 ticks vs 5 ticks per half-frame). Every
pair takes 8 ticks to send. Camera produces a half-frame every 5 ticks, so TX
occasionally catches up and enters TX REC (same bucket). The pair rotation
follows A+B → A+C → A+B → C+A → C+B → C+A → ... with the 8-tick pair cycle
drifting against the 10-tick frame cycle. Because TX is only slightly faster
than camera, TX REC appears frequently — TX often lands on the same bucket
camera is writing to.


---

### Scenario 6 — Camera 3 ticks, TX 7 ticks per half-frame (TX ~2.33× slower than camera)

| Tick | Cam          | TX    | A      | B      | C      | inA  | inB  | inC  |
|------|--------------|-------|--------|--------|--------|------|------|------|
|    0 | F0U->A 1/3   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    1 | F0U->A 2/3   | 'A+b  | TX REC | -      | -      | -    | -    | -    |
|    2 | F0U->A 3/3   | 'A+b  | TX REC | -      | -      | F0U  | -    | -    |
|    3 | F0L->B 1/3   | 'A+b  | TXP    | REC    | -      | F0U  | -    | -    |
|    4 | F0L->B 2/3   | 'A+b  | TXP    | REC    | -      | F0U  | -    | -    |
|    5 | F0L->B 3/3   | 'A+b  | TXP    | REC    | -      | F0U  | F0L  | -    |
|    6 | F1U->C 1/3   | 'A+b  | TXP    | PD     | REC    | F0U  | F0L  | -    |
|    7 | F1U->C 2/3   | 'a+B  | -      | TXP    | REC    | F0U  | F0L  | -    |
|    8 | F1U->C 3/3   | 'a+B  | -      | TXP    | REC    | F0U  | F0L  | F1U  |
|    9 | F1L->A 1/3   | 'a+B  | REC    | TXP    | -      | F0U  | F0L  | F1U  |
|   10 | F1L->A 2/3   | 'a+B  | REC    | TXP    | -      | F0U  | F0L  | F1U  |
|   11 | F1L->A 3/3   | 'a+B  | PD     | TXP    | -      | F1L  | F0L  | F1U  |
|   12 | F2U->C 1/3   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   13 | F2U->C 2/3   | 'a+B  | -      | TXP    | REC    | F1L  | F0L  | F1U  |
|   14 | F2U->C 3/3   | "C+a  | PD     | -      | TX REC | F1L  | F0L  | F2U  |
|   15 | F2L->B 1/3   | "C+a  | PD     | REC    | TXP    | F1L  | F0L  | F2U  |
|   16 | F2L->B 2/3   | "C+a  | PD     | REC    | TXP    | F1L  | F0L  | F2U  |
|   17 | F2L->B 3/3   | "C+a  | PD     | PD     | TXP    | F1L  | F2L  | F2U  |
|   18 | F3U->B 1/3   | "C+a  | PD     | REC    | TXP    | F1L  | F2L  | F2U  |
|   19 | F3U->B 2/3   | "C+a  | PD     | REC    | TXP    | F1L  | F2L  | F2U  |
|   20 | F3U->B 3/3   | "C+a  | PD     | REC    | TXP    | F1L  | F3U  | F2U  |
|   21 | F3L->C 1/3   | "c+A  | TXP    | -      | REC    | F1L  | F3U  | F2U  |
|   22 | F3L->C 2/3   | "c+A  | TXP    | -      | REC    | F1L  | F3U  | F2U  |
|   23 | F3L->C 3/3   | "c+A  | TXP    | -      | REC    | F1L  | F3U  | F3L  |
|   24 | F4U->B 1/3   | "c+A  | TXP    | REC    | -      | F1L  | F3U  | F3L  |
|   25 | F4U->B 2/3   | "c+A  | TXP    | REC    | -      | F1L  | F3U  | F3L  |
|   26 | F4U->B 3/3   | "c+A  | TXP    | REC    | -      | F1L  | F4U  | F3L  |
|   27 | F4L->C 1/3   | "c+A  | TXP    | -      | REC    | F1L  | F4U  | F3L  |
|   28 | F4L->C 2/3   | 'B+c  | -      | TXP    | REC    | F1L  | F4U  | F3L  |
|   29 | F4L->C 3/3   | 'B+c  | -      | TXP    | REC    | F1L  | F4U  | F4L  |
|   30 | F5U->A 1/3   | 'B+c  | REC    | TXP    | PD     | F1L  | F4U  | F4L  |
|   31 | F5U->A 2/3   | 'B+c  | REC    | TXP    | PD     | F1L  | F4U  | F4L  |
|   32 | F5U->A 3/3   | 'B+c  | REC    | TXP    | PD     | F5U  | F4U  | F4L  |
|   33 | F5L->A 1/3   | 'B+c  | REC    | TXP    | PD     | F5U  | F4U  | F4L  |
|   34 | F5L->A 2/3   | 'B+c  | REC    | TXP    | PD     | F5U  | F4U  | F4L  |
|   35 | F5L->A 3/3   | 'b+C  | REC    | -      | TXP    | F5L  | F4U  | F4L  |
|   36 | F6U->B 1/3   | 'b+C  | -      | REC    | TXP    | F5L  | F4U  | F4L  |
|   37 | F6U->B 2/3   | 'b+C  | -      | REC    | TXP    | F5L  | F4U  | F4L  |
|   38 | F6U->B 3/3   | 'b+C  | -      | REC    | TXP    | F5L  | F6U  | F4L  |
|   39 | F6L->A 1/3   | 'b+C  | REC    | -      | TXP    | F5L  | F6U  | F4L  |
|   40 | F6L->A 2/3   | 'b+C  | REC    | -      | TXP    | F5L  | F6U  | F4L  |
|   41 | F6L->A 3/3   | 'b+C  | REC    | -      | TXP    | F6L  | F6U  | F4L  |
|   42 | F7U->C 1/3   | "B+a  | PD     | TXP    | REC    | F6L  | F6U  | F4L  |
|   43 | F7U->C 2/3   | "B+a  | PD     | TXP    | REC    | F6L  | F6U  | F4L  |
|   44 | F7U->C 3/3   | "B+a  | PD     | TXP    | REC    | F6L  | F6U  | F7U  |
|   45 | F7L->C 1/3   | "B+a  | PD     | TXP    | REC    | F6L  | F6U  | F7U  |
|   46 | F7L->C 2/3   | "B+a  | PD     | TXP    | REC    | F6L  | F6U  | F7U  |
|   47 | F7L->C 3/3   | "B+a  | PD     | TXP    | REC    | F6L  | F6U  | F7L  |
|   48 | F8U->C 1/3   | "B+a  | PD     | TXP    | REC    | F6L  | F6U  | F7L  |
|   49 | F8U->C 2/3   | "b+A  | TXP    | -      | REC    | F6L  | F6U  | F7L  |
|   50 | F8U->C 3/3   | "b+A  | TXP    | -      | REC    | F6L  | F6U  | F8U  |
|   51 | F8L->B 1/3   | "b+A  | TXP    | REC    | -      | F6L  | F6U  | F8U  |
|   52 | F8L->B 2/3   | "b+A  | TXP    | REC    | -      | F6L  | F6U  | F8U  |
|   53 | F8L->B 3/3   | "b+A  | TXP    | REC    | -      | F6L  | F8L  | F8U  |
|   54 | F9U->C 1/3   | "b+A  | TXP    | -      | REC    | F6L  | F8L  | F8U  |
|   55 | F9U->C 2/3   | "b+A  | TXP    | -      | REC    | F6L  | F8L  | F8U  |
|   56 | F9U->C 3/3   | 'C+b  | -      | PD     | TX REC | F6L  | F8L  | F9U  |
|   57 | F9L->A 1/3   | 'C+b  | REC    | PD     | TXP    | F6L  | F8L  | F9U  |
|   58 | F9L->A 2/3   | 'C+b  | REC    | PD     | TXP    | F6L  | F8L  | F9U  |
|   59 | F9L->A 3/3   | 'C+b  | REC    | PD     | TXP    | F9L  | F8L  | F9U  |
|   60 | F10U->A 1/3  | 'C+b  | REC    | PD     | TXP    | F9L  | F8L  | F9U  |
|   61 | F10U->A 2/3  | 'C+b  | REC    | PD     | TXP    | F9L  | F8L  | F9U  |
|   62 | F10U->A 3/3  | 'C+b  | REC    | PD     | TXP    | F10U | F8L  | F9U  |
|   63 | F10L->C 1/3  | 'c+B  | -      | TXP    | REC    | F10U | F8L  | F9U  |
|   64 | F10L->C 2/3  | 'c+B  | -      | TXP    | REC    | F10U | F8L  | F9U  |
|   65 | F10L->C 3/3  | 'c+B  | -      | TXP    | REC    | F10U | F8L  | F10L |
|   66 | F11U->A 1/3  | 'c+B  | REC    | TXP    | -      | F10U | F8L  | F10L |
|   67 | F11U->A 2/3  | 'c+B  | REC    | TXP    | -      | F10U | F8L  | F10L |
|   68 | F11U->A 3/3  | 'c+B  | REC    | TXP    | -      | F11U | F8L  | F10L |
|   69 | F11L->C 1/3  | 'c+B  | -      | TXP    | REC    | F11U | F8L  | F10L |
|   70 | F11L->C 2/3  | "A+c  | TXP    | -      | REC    | F11U | F8L  | F10L |
|   71 | F11L->C 3/3  | "A+c  | TXP    | -      | REC    | F11U | F8L  | F11L |
|   72 | F12U->B 1/3  | "A+c  | TXP    | REC    | PD     | F11U | F8L  | F11L |
|   73 | F12U->B 2/3  | "A+c  | TXP    | REC    | PD     | F11U | F8L  | F11L |
|   74 | F12U->B 3/3  | "A+c  | TXP    | REC    | PD     | F11U | F12U | F11L |
|   75 | F12L->B 1/3  | "A+c  | TXP    | REC    | PD     | F11U | F12U | F11L |
|   76 | F12L->B 2/3  | "A+c  | TXP    | REC    | PD     | F11U | F12U | F11L |
|   77 | F12L->B 3/3  | "a+C  | -      | REC    | TXP    | F11U | F12L | F11L |
|   78 | F13U->A 1/3  | "a+C  | REC    | -      | TXP    | F11U | F12L | F11L |
|   79 | F13U->A 2/3  | "a+C  | REC    | -      | TXP    | F11U | F12L | F11L |
|   80 | F13U->A 3/3  | "a+C  | REC    | -      | TXP    | F13U | F12L | F11L |
|   81 | F13L->B 1/3  | "a+C  | -      | REC    | TXP    | F13U | F12L | F11L |
|   82 | F13L->B 2/3  | "a+C  | -      | REC    | TXP    | F13U | F12L | F11L |
|   83 | F13L->B 3/3  | "a+C  | -      | REC    | TXP    | F13U | F13L | F11L |
|   84 | F14U->C 1/3  | 'A+b  | TXP    | PD     | REC    | F13U | F13L | F11L |
|   85 | F14U->C 2/3  | 'A+b  | TXP    | PD     | REC    | F13U | F13L | F11L |
|   86 | F14U->C 3/3  | 'A+b  | TXP    | PD     | REC    | F13U | F13L | F14U |
|   87 | F14L->C 1/3  | 'A+b  | TXP    | PD     | REC    | F13U | F13L | F14U |
|   88 | F14L->C 2/3  | 'A+b  | TXP    | PD     | REC    | F13U | F13L | F14U |
|   89 | F14L->C 3/3  | 'A+b  | TXP    | PD     | REC    | F13U | F13L | F14L |

---

**Key observations — this is the richest scenario yet:**

**Tick 5→6:** Camera finishes F0L→B while TX still on A (TXP). B is the queued partner of the current pair → instantly becomes PD. This is the earliest PD appearance across all scenarios — camera is so fast it fills the partner slot while TX is still on the first bucket.

**Ticks 14→15: `"C+a` — Case 3 moment.** TX finishes B (second half of pair 1). Candidates: {A, C}. Camera is completing F2U (Upper) into C at tick 14 — C holds an Upper half in progress. A holds F1L (Lower half of frame 1), whose matching Upper (F1U) was already overwritten in C. No valid frame pair exists → Case 3. TX selects C (Upper) first, A second. Camera finishes C at tick 14 simultaneously with TX arriving → `TX REC`. A holds valid unsent data (F1L) and camera is not writing it, so A earns PD protection as the queued partner — camera must skip A until TX sends it.

**Ticks 30–34: A=REC, B=TXP, C=PD simultaneously** — all three buckets in a non-idle, non-free state at the same time. Camera is writing A (which it keeps overwriting since it can't go to B or C), B is being sent, C is queued protected. This is the "camera trapped on one bucket" scenario from the rules, demonstrated live.

**Ticks 27→28: `'B+c` — Case 2 moment.** TX finishes A (pair 2). Candidates {B, C}: B holds F4U (Upper, completed at tick 26). Camera is writing F4L (Lower, same frame 4) into C. Seamless matched pair → Case 2. TX selects B (Upper) first, C (Lower) second → `'B+c`.

**Ticks 41→42: `"B+a` — Case 1 moment.** Camera finishes F6L→A at tick 41, the exact same tick TX finishes C (pair 3). Candidates {A, B}: B=F6U (Upper), A=F6L (Lower) — a complete Frame 6 pair. TX immediately locks both: B (Upper) first, A (Lower) second → `"B+a`. Both buckets protected (B=TXP, A=PD), forcing camera to C.

**Ticks 55→56: `'C+b` — Case 3 moment.** TX finishes A (pair 4). Candidates {B, C}: Camera is writing F9U (Upper) into C. B holds F8L (orphaned Lower — its matching Upper F8U was overwritten). No valid pair → Case 3. TX selects C (Upper, in progress) first, B second → `'C+b`.

**Ticks 69→70: `"A+c` — Case 2 moment.** TX finishes B (pair 5). Candidates {A, C}: A holds F11U (Upper, completed at tick 68). Camera is writing F11L (Lower, same frame 11) into C. Seamless matched pair → Case 2. TX selects A (Upper) first, C (Lower) second → `"A+c`.

**Ticks 83→84: `'A+b` — Case 1 moment.** Camera finishes F13L→B at tick 83, the exact same tick TX finishes C (pair 6). Candidates {A, B}: A=F13U (Upper), B=F13L (Lower) — a complete Frame 13 pair. The 42-tick pattern (3 pairs × 14 ticks) repeats: Case 3 → Case 2 → Case 1.

**Sent pair results:**

| Completed at tick | Upper  | Lower  | Result         |
|-------------------|--------|--------|----------------|
| 13                | F0U ✓  | F0L ✓  | **Frame 0 ✓**  |
| 27                | F2U ✓  | F1L ✓  | mismatch       |
| 41                | F4U ✓  | F4L ✓  | **Frame 4 ✓**  |
| 55                | F6U ✓  | F6L ✓  | **Frame 6 ✓**  |
| 69                | F9U ✓  | F8L ✓  | mismatch       |
| 83                | F11U ✓ | F11L ✓ | **Frame 11 ✓** |

**Result:** Every 14 ticks TX completes a pair. The pair selection cycles through all three cases in a fixed 42-tick pattern: Case 3 → Case 2 → Case 1 → Case 3 → Case 2 → Case 1 → ... Case 1 (simultaneous finish) produces a matched frame; Case 2 (camera writing Lower) also produces a matched frame; Case 3 (camera writing Upper, orphaned Lower) produces a mismatch. Steady-state: 2 matched frames per 3 pairs (per 42 ticks, during which camera produces 7 frames). Drop rate is ~71%. Mismatches always contain valid data (both halves complete, just from different frames). This is the only scenario that demonstrates all three cases equally.

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


