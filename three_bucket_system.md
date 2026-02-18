
## Physical Model

* 3 buckets: A, B, C
* Each bucket holds exactly **one half-frame** (75 kB).
* Frame = Upper half + Lower half.
* Camera writes strictly:

```
Frame N Upper
Frame N Lower
Frame N+1 Upper
Frame N+1 Lower
...
```

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

---

# 🔹 Early Freeing Rule

When sending a pair (X, Y):

1. Send first bucket.
2. Immediately mark it FREE (camera may reuse it).
3. Send second bucket.
4. Mark it FREE after TX complete.

Only the currently transmitting bucket must be protected.

---

# 🔹 Why This Works

Because:

* Camera is strictly ordered (Upper → Lower).
* Camera is faster than Ethernet.
* At most one bucket is WRITING.
* The sender is always at least half a frame behind.

Therefore:

✔ We never transmit mismatched halves.
✔ We never block.
✔ Overwrites only drop old frames.
✔ Pipeline remains stable with only 3 half-buffers.

