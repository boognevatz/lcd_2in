Here’s the high-level architecture of this program.

**Overview**
- It is a MicroPython-based camera streaming server for an RP2350 + W5500 Ethernet + OV5640 camera.
- The design is split into:
  - Python for bootstrapping, HTTP routing, sensor/control logic, and browser UI
  - C in `modules/camera/*` for high-speed capture, DMA/PIO handling, and fast streaming
  - HTML tools like `stream_viewer.html` for offline inspection of the rich per-frame metadata

**Main Runtime Flow**
- `main_stream_semifinalist.py`
  - Initializes Ethernet on the W5500, camera startup, DAC/LED, temperature ADC, and barometer/I2C devices.
  - Creates the HTTP server socket on port 80 and runs the accept/request loop.
  - Parses the request line and delegates almost all endpoint behavior to `endpoints.handle_request(...)`.
  - Recreates the server socket after each handled request/stream teardown, which is a deliberate part of the stability model.

**HTTP / UI Layer**
- `endpoints.py`
  - Acts as the application/controller layer.
  - Serves `/` with a live HTML page that renders the stream in-browser and exposes controls for:
    - start/stop stream
    - LED brightness
    - runtime format/resolution switching
    - I2C register read/write
  - Routes stream endpoints like `/streamc`, `/streamc_rgb565`, `/streamc_720p`, `/streamc_1080p`.
  - For C streaming, `_streamc_common(...)` sends the multipart HTTP header, calls `camera.stream_start()`, then loops on `camera.stream_loop_c(...)`.
  - Between stream batches it also refreshes sensor data and pushes it into the camera module via setters so it can be emitted as per-frame headers.

**Camera Driver Stack**
- `modules/camera/modcamera.c`
  - This is the MicroPython extension module exposed as `import camera`.
  - It is the bridge between Python and the low-level camera engine.
  - Exposes camera control functions, sensor setters, register read/write wrappers, and the fast streaming entrypoints:
    - `camera.stream_start()`
    - `camera.stream_loop_c(...)`
  - Builds the multipart frame output and all `X-*` diagnostic headers.
  - Contains the TX-side scheduling logic: choosing which two buckets to send, wait behavior, timing metrics, protection state transitions, and metadata stamping.

- `modules/camera/cam.c`
  - This is the low-level capture engine.
  - Owns the 3-bucket memory ring, DMA channel chaining, IRQ handlers, VSYNC handling, and camera-side state machine.
  - Uses PIO + DMA to continuously capture camera data into half-frame buckets.
  - Updates atomic bucket state, frame/half counters, camera timing metrics, and protection/hint state shared with the TX logic.

- `modules/camera/cam.h`
  - Shared contract between capture logic and TX logic.
  - Defines:
    - bucket sizes and tagged frame layout
    - bucket state bitfields
    - TX protection states
    - shared globals like `bucket_state`, `cam_counter`, timing values, hints, etc.

**Sensor Configuration / Camera Register Programming**
- `ov5640_i2c.py`
  - Pure-Python OV5640 configuration layer.
  - Writes long register sequences for JPEG/RGB565 modes and different resolutions.
  - Used by startup and by runtime switching endpoints.
- `modules/camera/ov5640.c` and `modules/camera/ov5640.h`
  - Lower-level OV5640 support on the C side.
- `tempsensor.py`
  - Reads external temperature sensors over I2C.
- `barometer.py`
  - Reads barometer data and exposes pressure/temperature to the stream pipeline.

**Data Path**
- Capture path:
  - OV5640 sensor -> PIO camera input -> DMA -> 3 rotating half-frame buckets in `cam.c`
- Scheduling path:
  - `cam.c` marks bucket states complete/dirty and advances `cam_counter`
  - `modcamera.c` snapshots those states, chooses a pair to transmit, optionally waits, then stamps headers/tags
- Network path:
  - `modcamera.c` writes multipart boundaries + `X-*` headers + two tagged half-buffers to the client socket
  - `endpoints.py` only orchestrates the stream loop; the hot path is in C

**Three-Bucket Design**
- The capture/transmit pipeline is built around 3 buckets labeled A/B/C.
- At any moment:
  - one bucket may be actively sent
  - one may be queued/partially protected
  - one may be actively written by DMA
- This lets the camera and Ethernet TX overlap without requiring a full extra frame buffer.
- The bucket state machine and protection logic are the core of the project’s performance behavior.

**Metadata / Diagnostics**
- Each transmitted frame carries a large set of `X-*` headers from `modcamera.c`, including:
  - temperatures
  - barometer
  - free memory
  - bucket states before/after waits
  - TX state snapshots
  - upper/lower timing metrics for camera and TX
  - mode classification like camera-faster / camera-slower
- Each half-buffer also has a 20-byte binary tag embedded in front of payload data.

**Viewer / Debug Tools**
- `stream_viewer.html`
  - Offline viewer/parser for recorded multipart streams.
  - Decodes headers and binary tags to inspect bucket behavior, timing, hints, and scheduling decisions frame by frame.
- `tools/timeline_simulation.py`, `tools/tx_pair.py`, `tools/hint_sim.py`
  - Analysis/simulation helpers for the bucket scheduling logic.

**In Practice**
- `main_stream_semifinalist.py` is the app entrypoint.
- `endpoints.py` is the HTTP/router/UI layer.
- `ov5640_i2c.py`, `tempsensor.py`, `barometer.py` are device-specific Python helpers.
- `modules/camera/cam.c` is the capture engine.
- `modules/camera/modcamera.c` is the fast TX/metadata MicroPython module.
- `stream_viewer.html` is the forensic/debug frontend for recorded streams.

If you want, I can also turn this into a more structured “module map” with one short paragraph per file.
