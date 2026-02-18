# Three-Bucket Camera System — Full Analysis

## System Overview

RP2350 (Waveshare Touch LCD 2") running MicroPython with custom C modules.
OV5640 camera (240x320, RGB565) → PIO → DMA → RAM → W5500 Ethernet → browser.

Two camera module variants exist in the repo:

| | `camera/` (three_buckets branch) | `camera-good-colors/` (older) |
|---|---|---|
| Buffer scheme | 3 half-frame buckets | 2 full-frame double buffer |
| RAM | 3 x 76,800 = **230,400 bytes** | 2 x 153,600 = **307,200 bytes** |
| DMA channels | 2 (chained A↔B) | 1 |
| PIO | Standard (1 pixel/push, 16-bit) | Packed (2 pixels/push, 32-bit) |
| Frame detection | Half-frame granularity | Full-frame swap |
| Extra features | — | init_with_registers, write/read_register |
| Python API | `is_frame_ready()`, `frame()` → tuple of 2 | `is_buffer_ready()`, `frame()` → single bytearray |

The active build (`modules/micropython.cmake` line 10) includes `camera/` (three-bucket).

---

## DMA Chain Rotation — How It Actually Works

Two DMA channels (A, B) chain to each other. Each transfer = one half-frame (38,400 x 16-bit = 76,800 bytes). The ISR reconfigures the *completed* channel's write address to point 2 positions ahead in the rotation.

```
Step  Channel  Writes→    write_pos  Frame?   Pair offered
───────────────────────────────────────────────────────────
 1      A      bucket[0]    0→1       no
 2      B      bucket[1]    1→2       YES      [0] + [1]
 3      A      bucket[2]    2→3       no
 4      B      bucket[0]    3→4       YES      [2] + [0]
 5      A      bucket[1]    4→5       no
 6      B      bucket[2]    5→6       YES      [1] + [2]
 7      A      bucket[0]    6→7       no       (cycle repeats)
```

Result: strict round-robin `0 → 1 → 2 → 0 → 1 → 2 → ...`

Frame pair logic (`cam.c:170-177`):
- `frame_second_idx = completed_bucket`
- `frame_first_idx = (completed_bucket + 2) % 3`

This correctly identifies the two *consecutively written* buckets as upper + lower halves.
The pairing is correct: the "first" half was written one step before the "second" half.

---

## Spec vs. Code — What Matches, What Doesn't

### Matches the spec

- 3 buckets, each one half-frame (76,800 bytes) — `cam.c:51-54`
- Camera writes strictly Upper → Lower — enforced by PIO VSYNC gating
- Only one bucket WRITING at a time — DMA chain guarantees this
- Frame drops allowed — `cam.c:176` silently drops when `read_in_progress`
- Camera never stops — DMA chain is self-perpetuating, no blocking

### Does NOT match the spec

**1. Early Freeing Rule — NOT IMPLEMENTED**

The spec says (line 86-89):
> Send first bucket. Immediately mark it FREE. Send second bucket. Mark it FREE after TX.

The code does:
```c
cam_start_read();              // lock BOTH buckets
send(bucket[first_idx]);       // ~40ms
send(bucket[second_idx]);      // ~40ms
cam_end_read();                // unlock BOTH
```

Both buckets stay locked for the entire ~80ms transmission. This defeats the purpose of the third bucket.

**2. Speculative Pairing — NOT IMPLEMENTED**

The spec's Case I says start transmitting a bucket that's still being written, banking on the camera's write pointer always being ahead. The code only offers frames after both halves are *fully captured* (`write_pos & 1 == 0`).

**3. "Ethernet never waits" — PARTIALLY TRUE**

The C stream loop busy-polls `frame_ready`. Between frames, it spins. It doesn't block on a semaphore, but it does burn cycles waiting.

---

## The Critical Bug: DMA Overwrites During Read

`read_in_progress` only prevents the ISR from updating `frame_first_idx`/`frame_second_idx` and `frame_ready`. It does NOT stop DMA hardware from writing to the physical bucket memory.

**Concrete scenario:**

1. Frame ready at write_pos=2 → pair is `bucket[0]` + `bucket[1]`
2. Python calls `cam_start_read()`, starts sending `bucket[0]`
3. DMA writes `bucket[2]` (write_pos → 3) — **SAFE**
4. DMA writes `bucket[0]` (write_pos → 4) — **CORRUPTION** if still reading bucket[0]
5. DMA writes `bucket[1]` (write_pos → 5) — **CORRUPTION** if still reading bucket[1]

Steps 4-5 happen ~120ms after step 1 (two half-frame capture periods). If ethernet transmission takes longer than ~60ms for the first half, `bucket[0]` gets overwritten mid-read.

**Why it probably works in practice:**

With the W5500 at 40MHz SPI, sending 76,800 bytes takes roughly 30-40ms. The camera half-frame at OV5640 PLL settings appears to be ~60ms. So the read finishes ~40ms before the DMA loops back. It's a timing race with margin, not a hard guarantee.

**The fix** (not implemented): Early freeing. After sending the first half, set a flag so the ISR knows that bucket is available. The third bucket exists precisely to absorb this overlap — but the code doesn't use it.

---

## Missing Exception Safety in stream_loop_c

The three-bucket `camera_stream_loop_c` (modcamera.c:203-311) has no `nlr_push`/`nlr_pop` guard around the send-while-locked section:

```c
cam_start_read();
// ... mp_stream_write_exactly() could raise ...
cam_end_read();    // never reached if exception thrown
```

If an exception escapes `mp_stream_write_exactly`, `read_in_progress` stays `true` forever. Every subsequent ISR invocation drops the frame at `cam.c:171`. The camera stream is permanently dead until reset.

The `camera-good-colors/modcamera.c` version DOES have this guard (lines 193-229 with `nlr_buf_t`).

---

## Python Script Variants

| Script | Stream Path | Method | Notes |
|--------|-------------|--------|-------|
| `main_stream_l.py` | `/stream` | Python loop + callback | Oldest, uses `is_buffer_ready()` (wrong API for three-bucket module) |
| `main_stream_l_fixed.py` | `/stream` | Python loop + callback | Same as above, uses `is_frame_ready()` (correct API) |
| `main_stream_l_cloop.py` | `/stream` + `/streamc` | Python loop OR C loop | Most complete, `/streamc` uses `stream_loop_c()` |
| `main_stream_g.py` | `/stream` | Non-blocking poll + `send_frame_data_c()` | Per-frame from Python, has config.json support |

**API mismatch bug:** `main_stream_l.py` calls `camera.is_buffer_ready()` (line 495) which only exists in the `camera-good-colors` module. On the three-bucket module, this would raise `AttributeError` at runtime. The correct API is `camera.is_frame_ready()`.

---

## Memory Layout

```
Static RAM allocation (three-bucket):
  bucket_mem_0:  76,800 bytes  (half-frame upper)
  bucket_mem_1:  76,800 bytes  (half-frame lower)
  bucket_mem_2:  76,800 bytes  (overlap absorber)
  TOTAL:        230,400 bytes

Savings vs double-buffer: 76,800 bytes (one full half-frame)
```

The three-bucket design saves ~75 KB of RAM, which on an RP2350 with 520 KB SRAM is significant (~14% of total).

---

## PIO Differences

**`camera/` (three-bucket)** uses `picampinos.pio`:
- 1 pixel per FIFO push
- Manual `push` instruction
- DMA_SIZE_16 (16-bit transfers)
- Standard byte order

**`camera-good-colors/`** uses inline PIO in `cam.c`:
- 2 pixels per FIFO push (packed 32-bit)
- Auto-push at 31 bits (workaround for RP2350 encoding bug at 32)
- DMA_SIZE_32 (32-bit transfers)
- Swapped pixel pairs in memory → JS must fix byte order

The packed PIO is more efficient (half the FIFO pushes, half the DMA transfers), but requires JS-side byte swapping and makes the half-frame split impossible (pixel pairs are interleaved).

---

## Summary of Issues (ranked by severity)

1. **DMA overwrite race** — No hard protection for buckets during read. Works by timing margin only. Severity: medium (probably fine at current speeds, but fragile).

2. **No exception safety in stream_loop_c** — Missing `nlr_push`/`nlr_pop`. An exception during send permanently kills streaming. Severity: medium.

3. **Early freeing not implemented** — The third bucket's purpose is wasted. Both read buckets locked for full TX duration. Severity: low-medium (reduces the timing safety margin).

4. **API mismatch in main_stream_l.py** — Calls `is_buffer_ready()` instead of `is_frame_ready()`. Will crash on the three-bucket module. Severity: low (only matters if someone runs that specific script).

5. **Spec overreaches** — The "speculative pairing" concept (transmit mid-capture) is not implemented and would require careful pointer tracking. The current implementation is simpler and safer. This is not a bug, just a spec/code gap.
