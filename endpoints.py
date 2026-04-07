"""
HTTP endpoint handlers - extracted from main_stream_semifinalist.py
"""

import machine
import network
import socket
import time
import camera
import gc
import tempsensor
import barometer
import json
import ov5640_i2c

# DEBUG flag - set from main module
DEBUG = False

def set_debug(enabled):
    """Set debug mode from main module"""
    global DEBUG
    DEBUG = enabled

if DEBUG:
    def debug_print(*args, **kwargs):
        print(*args, **kwargs)
else:
    def debug_print(*args, **kwargs):
        pass


# ============== HELPER FUNCTIONS ==============

def generate_html_root():
    """path: /"""
    gc.collect()
    html = """HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<!DOCTYPE html>
<html>
<head>
    <title>Camera Stream</title>
    <style>
        body { margin: 20px; font-family: Arial, sans-serif; background: #222; color: #fff; }
        #camera-canvas { border: 2px solid #000; display: block; }
        #stats { margin-top: 10px; font-size: 16px; }
        .metric { display: inline-block; margin-right: 20px; padding: 5px 10px; background: #333; }
        #controls { margin-top: 10px; }
        #controls button { padding: 8px 16px; margin-right: 10px; font-size: 14px; cursor: pointer; }
        #btn-stream { background: #c33; color: #fff; border: none; }
        #btn-stream.stopped { background: #3a3; }
        .btn-led { background: #555; color: #fff; border: none; }
        .btn-led:disabled { opacity: 0.4; cursor: not-allowed; }
        .btn-led.active { background: #c90; }
        .btn-stream-mode { background: #555; color: #fff; border: none; }
        #color-format-options { margin-top: 10px; }
        #color-format-options label { display: block; margin: 5px 0; }
        
        /* New View Sizing Styles */
        #camera-canvas { border: 2px solid #000; display: block; max-width: 100%; max-height: 80vh; width: auto; height: auto; object-fit: contain; }
        .view-btn { background: #444; color: #fff; border: 1px solid #666; padding: 4px 10px; cursor: pointer; font-size: 12px; margin-right: 4px; border-radius: 3px; }
        .view-btn.active { background: #3b82f6; border-color: #2563eb; }
        #canvas-wrap { position: relative; display: inline-flex; justify-content: center; align-items: center; overflow: hidden; max-height: 80vh; max-width: 100%; }
        #img-dim { position: absolute; top: 8px; left: 8px; background: rgba(0,0,0,0.65); color: #fff; padding: 4px 6px; border-radius: 4px; font-size: 11px; font-family: monospace; pointer-events: none; z-index: 10; }
    </style>
</head>
<body>
    <h2>Camera Stream - Auto-Detect (JPEG/RGB565)</h2>
    <div id="view-controls" style="margin-bottom: 10px;">
        <span style="font-size: 14px; margin-right: 8px;">View Size:</span>
        <button class="view-btn active" data-size="auto">Auto Fit</button>
        <button class="view-btn" data-size="native">1:1 Native</button>
        <button class="view-btn" data-size="640x480">640x480</button>
        <button class="view-btn" data-size="1280x720">720p</button>
        <button class="view-btn" data-size="1920x1080">1080p</button>
    </div>
    <div id="canvas-wrap">
        <div id="img-dim">Waiting...</div>
        <canvas id="camera-canvas" width="240" height="320"></canvas>
    </div>
    <div id="stats">
        <div class="metric">FPS: <span id="fps">0.00</span></div>
        <div class="metric">Frame: <span id="frame-count">0</span></div>
        <div class="metric">Status: <span id="status">Starting...</span></div>
        <div class="metric">Dropped halves: <span id="dropped-halves">0</span></div>
        <div class="metric">Mem Free: <span id="mem-free">N/A</span></div>
    </div>
    <div id="controls">
        <button id="btn-stream" onclick="toggleStream()">Stop Stream</button>
        <button class="btn-led" onclick="sendLedCommand(0)">LED 0%</button>
        <button class="btn-led" onclick="sendLedCommand(8)">LED 8%</button>
        <button class="btn-led" onclick="sendLedCommand(10)">LED 10%</button>
        <button class="btn-led" onclick="sendLedCommand(15)">LED 15%</button>
        <button class="btn-led" onclick="sendLedCommand(20)">LED 20%</button>
        <button class="btn-led" onclick="sendLedCommand(100)">LED 100%</button>
    </div>
    <div id="controls">
        <strong>Direct Stream:</strong>
        <button class="btn-stream-mode" onclick="switchStream('/streamc_vga')">VGA JPEG</button>
        <button class="btn-stream-mode" onclick="switchStream('/streamc_rgb565')">RGB565</button>
        <button class="btn-stream-mode" onclick="switchStream('/streamc_720p')">720p JPEG</button>
        <button class="btn-stream-mode" onclick="switchStream('/streamc_1080p')">1080p JPEG</button>
    </div>
    <div id="controls">
        <strong>Runtime Switch:</strong>
        <button class="btn-res" onclick="setCameraFormat('jpeg', 'vga')">VGA JPEG</button>
        <button class="btn-res" onclick="setCameraFormat('jpeg', '720p')">720p JPEG</button>
        <button class="btn-res" onclick="setCameraFormat('jpeg', '1080p')">1080p JPEG</button>
        <button class="btn-res" onclick="setCameraFormat('rgb565', 'vga')">RGB565</button>
    </div>
    <div id="i2c-controls" style="margin-top: 10px; padding: 10px; border: 1px solid #555;">
        <strong>I2C Register Tuning:</strong><br>
        <label>Address: <input type="text" id="reg-address" value="0x4407" size="6" onkeydown="if(event.key === 'Enter') readI2CRegister()"></label>
        <label style="margin-left: 10px;">Value: <input type="text" id="reg-value" value="0x12" size="4" onkeydown="if(event.key === 'Enter') setI2CRegister()"></label>
        <button onclick="readI2CRegister()">Read Register</button>
        <button onclick="setI2CRegister()">Set Register</button>
        <div style="margin-top: 5px; font-size: 12px; color: #aaa;">
            (eg. 0x08-0x40, high quality: 0x08-0x10, low quality: 0x20-0x40)
        </div>
    </div>
    <div id="color-format-options">
        <strong>Color Format:</strong><br>
        <label><input type="radio" name="color-format" value="bgr"> b01234_g012345_r01234</label>
        <label><input type="radio" name="color-format" value="rgb" checked> r01234_g012345_b01234</label>
        <label><input type="radio" name="color-format" value="grb"> g01234_r012345_b01234</label>
        <label><input type="radio" name="color-format" value="brg"> b01234_r012345_g01234</label>
        <label><input type="radio" name="color-format" value="gbr"> g01234_b012345_r01234</label>
        <label><input type="radio" name="color-format" value="rbg"> r01234_b012345_g01234</label>
    </div>
    <script>
        // ---- View Sizing Logic ----
        const viewBtns = document.querySelectorAll('.view-btn');
        const canvasWrap = document.getElementById('canvas-wrap');
        const imgDim = document.getElementById('img-dim');
        let currentViewMode = 'auto';

        function formatBytes(bytes, decimals = 2) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const dm = decimals < 0 ? 0 : decimals;
            const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
        }

        function applyViewSize() {
            canvasWrap.style.overflow = 'hidden';
            canvasWrap.style.maxHeight = '80vh';
            canvas.style.maxWidth = '100%';
            canvas.style.maxHeight = '80vh';
            canvas.style.width = 'auto';
            canvas.style.height = 'auto';
            canvas.style.objectFit = 'contain';

            if (currentViewMode !== 'auto') {
                canvasWrap.style.overflowX = 'auto';
                canvasWrap.style.overflowY = 'hidden';
                canvasWrap.style.maxHeight = 'none';
                canvas.style.maxWidth = 'none';
                canvas.style.maxHeight = 'none';

                if (currentViewMode === 'native') {
                    canvas.style.width = canvas.width + 'px';
                    canvas.style.height = canvas.height + 'px';
                } else {
                    const parts = currentViewMode.split('x');
                    canvas.style.width = parts[0] + 'px';
                    canvas.style.height = parts[1] + 'px';
                }
            }

            viewBtns.forEach(b => {
                if (b.dataset.size === currentViewMode) {
                    b.classList.add('active');
                } else {
                    b.classList.remove('active');
                }
            });
        }

        viewBtns.forEach(b => {
            b.addEventListener('click', (e) => {
                currentViewMode = e.target.dataset.size;
                applyViewSize();
            });
        });
        
        const canvas = document.getElementById('camera-canvas');
        const ctx = canvas.getContext('2d');
        const width = 240;
        const height = 320;
        const TAG_SIZE = 20;  // 20-byte tag per half-frame (time_us + bucket tag + tx states + hints + isr_time)
        const rgb565HalfBytes = width * (height / 2) * 2; // 76800
        const rgb565FrameSize = width * height * 2 + TAG_SIZE * 2; // 153640

        let frameCount = 0;
        let fpsCounter = 0;
        let lastFpsTime = Date.now();
        let droppedHalfFrames = 0;

        // Single-socket guard: only one active request at a time
        let requestInProgress = false;
        let abortController = null;
        let streamEnabled = true;
        let commandPending = null;  // URL to fetch after stream stops
        let streamUrl = '/streamc';  // Current stream endpoint URL

        function displayImage(arrayBuffer) {
            const data = new Uint8Array(arrayBuffer);
            const halfPayloadBytes = Math.max(0, (data.length - TAG_SIZE * 2) >> 1);

            // --- Extract frame numbers from 20-byte tags ---
            // Tag layout: [time_us(4B), bucket_state(4B), tx_states(4B), hints(4B), isr_time(4B)]
            // bucket_state: bits 31-3 = frame number, bit 2 = half, bit 1 = dirty, bit 0 = valid
            // Upper tag at offset 0, lower tag at offset TAG_SIZE + half payload bytes
            const upperStateOffset = 4; // bytes 4-7 of upper tag
            const lowerTagStart = TAG_SIZE + halfPayloadBytes;
            const lowerStateOffset = lowerTagStart + 4; // bytes 4-7 of lower tag

            const upperState = data[upperStateOffset]
                | (data[upperStateOffset + 1] << 8)
                | (data[upperStateOffset + 2] << 16)
                | (data[upperStateOffset + 3] << 24);
            const lowerState = data[lowerStateOffset]
                | (data[lowerStateOffset + 1] << 8)
                | (data[lowerStateOffset + 2] << 16)
                | (data[lowerStateOffset + 3] << 24);

            const upperFrameNum = upperState >>> 3;
            const lowerFrameNum = lowerState >>> 3;

            if (upperFrameNum !== lowerFrameNum) {
                // Mismatched halves — drop this frame, count both halves
                droppedHalfFrames += 2;
                document.getElementById('dropped-halves').textContent = droppedHalfFrames;
                return;
            }

            function updateStats() {
                frameCount++;
                fpsCounter++;
                document.getElementById('frame-count').textContent = frameCount;

                const now = Date.now();
                if (now - lastFpsTime >= 1000) {
                    const fps = fpsCounter / ((now - lastFpsTime) / 1000);
                    document.getElementById('fps').textContent = fps.toFixed(2);
                    fpsCounter = 0;
                    lastFpsTime = now;
                }
            }

            // Auto-detect JPEG by looking for SOI marker (FF D8)
            if (data[TAG_SIZE] === 0xFF && data[TAG_SIZE + 1] === 0xD8) {
                const jpegData = new Uint8Array(halfPayloadBytes * 2);
                jpegData.set(data.subarray(TAG_SIZE, TAG_SIZE + halfPayloadBytes), 0);
                jpegData.set(
                    data.subarray(TAG_SIZE * 2 + halfPayloadBytes, TAG_SIZE * 2 + halfPayloadBytes * 2),
                    halfPayloadBytes,
                );

                let eoiIndex = jpegData.length;
                for (let i = jpegData.length - 2; i >= 0; i--) {
                    if (jpegData[i] === 0xFF && jpegData[i+1] === 0xD9) {
                        eoiIndex = i + 2;
                        break;
                    }
                }
                
                const blob = new Blob([jpegData.subarray(0, eoiIndex)], { type: 'image/jpeg' });
                const url = URL.createObjectURL(blob);
                const img = new Image();
                img.onload = () => {
                    if (canvas.width !== img.naturalWidth || canvas.height !== img.naturalHeight) {
                        canvas.width = img.naturalWidth;
                        canvas.height = img.naturalHeight;
                    }
                    imgDim.innerHTML = `<b>${img.naturalWidth}</b> x <b>${img.naturalHeight}</b> JPEG<br><span style="opacity:0.7">${formatBytes(eoiIndex)}</span>`;
                    ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
                    URL.revokeObjectURL(url);
                    applyViewSize();
                    updateStats();
                };
                img.src = url;
                return;
            }

            // --- RGB565 Rendering ---
            if (halfPayloadBytes < rgb565HalfBytes) {
                return;
            }

            if (canvas.width !== width || canvas.height !== height) {
                canvas.width = width;
                canvas.height = height;
            }
            const imageData = ctx.createImageData(width, height);
            const halfPixels = width * (height / 2);

            // Upper half: skip 20-byte tag, read 76800 bytes
            let si = TAG_SIZE;
            let di = 0;
            for (let i = 0; i < halfPixels; i++) {
                if (si + 1 >= data.length) break;
                const v = (data[si + 1] << 8) | data[si];
                const r5 = (v >> 11) & 0x1F;
                const g6 = (v >> 5) & 0x3F;
                const b5 = v & 0x1F;
                imageData.data[di]     = (r5 << 3) | (r5 >> 2);
                imageData.data[di + 1] = (g6 << 2) | (g6 >> 4);
                imageData.data[di + 2] = (b5 << 3) | (b5 >> 2);
                imageData.data[di + 3] = 255;
                si += 2;
                di += 4;
            }

            // Lower half: skip another 20-byte tag, read 76800 bytes
            si += TAG_SIZE;
            for (let i = 0; i < halfPixels; i++) {
                if (si + 1 >= data.length) break;
                const v = (data[si + 1] << 8) | data[si];
                const r5 = (v >> 11) & 0x1F;
                const g6 = (v >> 5) & 0x3F;
                const b5 = v & 0x1F;
                imageData.data[di]     = (r5 << 3) | (r5 >> 2);
                imageData.data[di + 1] = (g6 << 2) | (g6 >> 4);
                imageData.data[di + 2] = (b5 << 3) | (b5 >> 2);
                imageData.data[di + 3] = 255;
                si += 2;
                di += 4;
            }

            imgDim.innerHTML = `<b>${width}</b> x <b>${height}</b> RGB565<br><span style="opacity:0.7">Raw uncompressed</span>`;
            ctx.putImageData(imageData, 0, 0);
            applyViewSize();
            updateStats();
        }

        function toggleStream() {
            const btn = document.getElementById('btn-stream');
            if (streamEnabled) {
                // Stop
                streamEnabled = false;
                btn.textContent = 'Start Stream';
                btn.classList.add('stopped');
                if (abortController) abortController.abort();
            } else {
                // Start
                streamEnabled = true;
                btn.textContent = 'Stop Stream';
                btn.classList.remove('stopped');
                startStream();
            }
        }

        async function sendLedCommand(percent) {
            const btns = document.querySelectorAll('.btn-led');
            btns.forEach(b => b.disabled = true);
            const wasStreaming = streamEnabled;

            // Stop stream first (frees the single socket)
            if (abortController) {
                streamEnabled = false;
                abortController.abort();
            }

            // Wait for MCU to close the stream socket
            await new Promise(r => setTimeout(r, 600));

            // Send command on the now-free socket
            try {
                document.getElementById('status').textContent = 'Sending LED command...';
                const resp = await fetch('/headled/' + percent);
                const text = await resp.text();
                document.getElementById('status').textContent = 'LED: ' + text;
            } catch (err) {
                document.getElementById('status').textContent = 'LED error: ' + err.message;
            }

            // Wait for MCU to close command socket and recreate listener
            await new Promise(r => setTimeout(r, 600));

            btns.forEach(b => { b.disabled = false; b.classList.remove('active'); });
            btns.forEach(b => { if (b.textContent === 'LED ' + percent + '%') b.classList.add('active'); });

            // Restart stream if it was running before
            if (wasStreaming) {
                streamEnabled = true;
                const sbtn = document.getElementById('btn-stream');
                sbtn.textContent = 'Stop Stream';
                sbtn.classList.remove('stopped');
                startStream();
            }
        }

        async function readI2CRegister() {
            const addr = document.getElementById('reg-address').value;
            const btn = document.querySelector('button[onclick="readI2CRegister()"]');
            
            if (btn) btn.disabled = true;
            const wasStreaming = streamEnabled;

            if (abortController) {
                streamEnabled = false;
                abortController.abort();
            }

            await new Promise(r => setTimeout(r, 600));

            try {
                document.getElementById('status').textContent = 'Reading I2C...';
                const response = await fetch(`/readi2cregister?address=${addr}`);
                const text = await response.text();
                if (response.ok) {
                    document.getElementById('reg-value').value = text;
                    document.getElementById('status').textContent = `I2C Read OK`;
                } else {
                    document.getElementById('status').textContent = `I2C Read Error: ${text}`;
                }
            } catch (err) {
                document.getElementById('status').textContent = 'I2C Read failed: ' + err.message;
            }

            await new Promise(r => setTimeout(r, 600));

            if (btn) btn.disabled = false;

            if (wasStreaming) {
                streamEnabled = true;
                const sbtn = document.getElementById('btn-stream');
                sbtn.textContent = 'Stop Stream';
                sbtn.classList.remove('stopped');
                startStream();
            }
        }

        async function setI2CRegister() {
            const addr = document.getElementById('reg-address').value;
            const val = document.getElementById('reg-value').value;
            const btn = document.querySelector('button[onclick="setI2CRegister()"]');
            
            if (btn) btn.disabled = true;
            const wasStreaming = streamEnabled;

            // Stop stream first (frees the single socket)
            if (abortController) {
                streamEnabled = false;
                abortController.abort();
            }

            // Wait for MCU to close the stream socket
            await new Promise(r => setTimeout(r, 600));

            try {
                document.getElementById('status').textContent = 'Setting I2C...';
                const response = await fetch(`/seti2cregister?address=${addr}&value=${val}`);
                const text = await response.text();
                if (response.ok) {
                    document.getElementById('status').textContent = `I2C: ${text}`;
                } else {
                    document.getElementById('status').textContent = `I2C Error: ${text}`;
                }
            } catch (err) {
                document.getElementById('status').textContent = 'I2C Request failed: ' + err.message;
            }

            // Wait for MCU to close command socket and recreate listener
            await new Promise(r => setTimeout(r, 600));

            if (btn) btn.disabled = false;

            // Restart stream if it was running before
            if (wasStreaming) {
                streamEnabled = true;
                const sbtn = document.getElementById('btn-stream');
                sbtn.textContent = 'Stop Stream';
                sbtn.classList.remove('stopped');
                startStream();
            }
        }

        async function setCameraFormat(format, res) {
            const btns = document.querySelectorAll('.btn-res');
            btns.forEach(b => b.disabled = true);
            const wasStreaming = streamEnabled;

            if (abortController) {
                streamEnabled = false;
                abortController.abort();
            }

            await new Promise(r => setTimeout(r, 600));

            try {
                document.getElementById('status').textContent = 'Switching format...';
                const resp = await fetch('/set_format/' + format + '/' + res);
                const text = await resp.text();
                document.getElementById('status').textContent = 'Format: ' + text;
            } catch (err) {
                document.getElementById('status').textContent = 'Format error: ' + err.message;
            }

            await new Promise(r => setTimeout(r, 600));

            btns.forEach(b => b.disabled = false);

            if (wasStreaming) {
                streamEnabled = true;
                const sbtn = document.getElementById('btn-stream');
                sbtn.textContent = 'Stop Stream';
                sbtn.classList.remove('stopped');
                startStream();
            }
        }

        async function switchStream(url) {
            streamUrl = url;
            const wasStreaming = streamEnabled;

            // Stop current stream
            if (abortController) {
                streamEnabled = false;
                abortController.abort();
            }

            // Wait for MCU to close the stream socket
            await new Promise(r => setTimeout(r, 600));

            // Highlight active button
            document.querySelectorAll('.btn-stream-mode').forEach(b => {
                b.style.background = b.getAttribute('onclick').includes(url) ? '#c90' : '#555';
            });

            // Restart stream on new endpoint
            if (wasStreaming) {
                streamEnabled = true;
                const sbtn = document.getElementById('btn-stream');
                sbtn.textContent = 'Stop Stream';
                sbtn.classList.remove('stopped');
                startStream();
            }
        }

        async function startStream() {
            if (!streamEnabled) return;

            // Single-socket guard: block concurrent requests
            if (requestInProgress) {
                console.log('Request already in progress, skipping');
                return;
            }

            // Abort any lingering previous request
            if (abortController) {
                abortController.abort();
                await new Promise(r => setTimeout(r, 100));
            }

            abortController = new AbortController();
            requestInProgress = true;

            try {
                document.getElementById('status').textContent = 'Connecting...';
                const response = await fetch(streamUrl, { signal: abortController.signal });
                document.getElementById('status').textContent = 'Streaming';

                const reader = response.body.getReader();
                let buffer = new Uint8Array(0);
                const boundaryText = '--frame';
                const boundary = new TextEncoder().encode(boundaryText);

                while (true) {
                    const {done, value} = await reader.read();

                    if (done) {
                        console.log('Stream ended');
                        document.getElementById('status').textContent = 'Stream ended';
                        break;
                    }

                    // Append new data to buffer
                    const newBuffer = new Uint8Array(buffer.length + value.length);
                    newBuffer.set(buffer);
                    newBuffer.set(value, buffer.length);
                    buffer = newBuffer;

                    // Process all complete frames in buffer
                    while (true) {
                        // Find boundary marker
                        let boundaryIndex = -1;
                        for (let i = 0; i < buffer.length - boundary.length; i++) {
                            let match = true;
                            for (let j = 0; j < boundary.length; j++) {
                                if (buffer[i + j] !== boundary[j]) {
                                    match = false;
                                    break;
                                }
                            }
                            if (match) {
                                boundaryIndex = i;
                                break;
                            }
                        }

                        if (boundaryIndex === -1) break;

                        // Find data start (after \\r\\n\\r\\n)
                        let dataStart = -1;
                        for (let i = boundaryIndex; i < buffer.length - 3; i++) {
                            if (buffer[i] === 13 && buffer[i+1] === 10 &&
                                buffer[i+2] === 13 && buffer[i+3] === 10) {
                                dataStart = i + 4;
                                break;
                            }
                        }

                        const headerBytes = buffer.slice(boundaryIndex, dataStart);
                        const headerText = new TextDecoder().decode(headerBytes);
                        const contentLengthMatch = headerText.match(/Content-Length:\s*(\d+)/i);
                        const contentLength = contentLengthMatch ? parseInt(contentLengthMatch[1], 10) : rgb565FrameSize;

                        const memFreeMatch = headerText.match(/X-MemFree:\s*([\d\s]+)/i);
                        if (memFreeMatch) {
                            document.getElementById('mem-free').textContent = memFreeMatch[1].trim();
                        }

                        if (!Number.isFinite(contentLength) || buffer.length - dataStart < contentLength) {
                            break; // Not enough data yet
                        }

                        // Extract and display frame
                        const frameData = buffer.slice(dataStart, dataStart + contentLength);
                        displayImage(frameData);

                        // Remove processed data
                        buffer = buffer.slice(dataStart + contentLength);
                    }
                }
            } catch (err) {
                if (err.name === 'AbortError') {
                    console.log('Request aborted');
                    document.getElementById('status').textContent = 'Stopped';
                } else {
                    console.error('Stream error:', err);
                    document.getElementById('status').textContent = 'Error: ' + err.message;
                }
            } finally {
                requestInProgress = false;
                abortController = null;
                // Only auto-reconnect if stream is enabled
                if (streamEnabled) {
                    document.getElementById('status').textContent = 'Reconnecting...';
                    setTimeout(startStream, 500);
                }
            }
        }

        // Start streaming
        startStream();
    </script>
</body>
</html>
"""
    return html.encode()


def generate_html_404():
    """path: /404"""
    html = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n404 Not Found"
    return html.encode()


# ============== ENDPOINT HANDLERS ==============

def handle_root():
    """Handle / endpoint - Serve HTML root page"""
    debug_print("[HANDLER] / html stream")
    return generate_html_root()


def handle_404():
    """Handle 404 for unknown paths"""
    debug_print("[HANDLER] 404")
    return generate_html_404()


def handle_headled(path, set_head_led_brightness_fn):
    """Handle /headled/{value} endpoint - Set LED brightness"""
    try:
        percent = int(path.split('/')[-1])
        ok = set_head_led_brightness_fn(percent)
        if ok:
            body = f"OK {percent}%"
        else:
            body = "DAC not ready"
    except ValueError:
        body = "Bad value"
    response = f"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    return response


def handle_getmcutemperature():
    """Handle /getmcutemperature endpoint - Get MCU temperature"""
    temp = tempsensor.get_mcu_temperature()
    body = json.dumps({"mcu_temperature": temp})
    response = f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    return response


def handle_gettemperature_all():
    """Handle /gettemperature/all endpoint - Get all temperature sensors"""
    if tempsensor.adc_ready:
        temps = tempsensor.read_all_temperatures()
        body = json.dumps(temps)
    else:
        body = json.dumps({"error": "ADC not ready"})
    response = f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    return response


def handle_gettemperature(path):
    """Handle /gettemperature/{id} endpoint - Get specific temperature sensor"""
    try:
        sensor_id = int(path.split('/')[-1])
        if sensor_id < 1 or sensor_id > 6:
            body = json.dumps({"error": "Invalid sensor ID (1-6)"})
            return f"HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
        elif not tempsensor.adc_ready:
            body = json.dumps({"error": "ADC not ready"})
            return f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
        else:
            # sensor_id 1-6 maps to channels 0-5
            voltage = tempsensor.read_adc_channel(sensor_id - 1)
            temp_c = tempsensor.voltage_to_temperature(voltage)
            body = json.dumps({f"headTemp{sensor_id}": {"raw_v": voltage, "temp_c": temp_c}})
            return f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    except ValueError:
        body = json.dumps({"error": "Invalid sensor ID"})
        return f"HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()


# ============== STREAMING ENDPOINT HANDLERS ==============
# These handle the connection loop and socket recreation themselves

def handle_stream(cl, s, create_server_socket_fn):
    """Handle /stream endpoint - PYTHON-BASED STREAMING
    
    This handler manages the entire streaming lifecycle including
    socket recreation on exit. Returns 'streaming' to indicate the
    caller should continue to the next connection without sending
    a response.
    """
    try:
        header = b"HTTP/1.1 200 OK\r\n"
        header += b"Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        header += b"Cache-Control: no-cache\r\n"
        header += b"Connection: keep-alive\r\n"
        header += b"\r\n"
        cl.send(header)
        debug_print("Stream client connected - header sent")
    except OSError as e:
        print(f"Failed to send header: {e}")
        cl.close()
        # Recreate socket and continue
        s.close()
        time.sleep_ms(100)
        gc.collect()
        s = create_server_socket_fn()
        return s, "continue"
    
    # Streaming loop entirely in Python
    state = {'streaming': True, 'frame_count': 0, 'first_frame': True}
    boundary = b"--frame\r\nContent-Type: application/octet-stream\r\n\r\n"
    frame_boundary = b"\r\n--frame\r\nContent-Type: application/octet-stream\r\n\r\n"
    cl.settimeout(5.0)
    
    def send_frame_data(frame_halves):
        try:
            if state['first_frame']:
                cl.send(boundary)
                state['first_frame'] = False
            else:
                cl.send(frame_boundary)
            chunk_size = 16384  # 16KB chunks
            for frame_data in frame_halves:
                mv = memoryview(frame_data)
                total_sent = 0
                while total_sent < len(frame_data):
                    end = min(total_sent + chunk_size, len(frame_data))
                    chunk = mv[total_sent:end]
                    try:
                        bytes_sent = cl.send(chunk)
                        if bytes_sent == 0:
                            state['streaming'] = False
                            return
                        total_sent += bytes_sent
                    except OSError as e:
                        debug_print(f"Stream send error: {e}")
                        state['streaming'] = False
                        return
            state['frame_count'] += 1
            if state['frame_count'] % 30 == 0:
                debug_print(f"Streamed {state['frame_count']} frames")
        except OSError as e:
            debug_print(f"Stream error: {e}")
            state['streaming'] = False
    
    debug_print("Starting Python-based streaming loop")
    while state['streaming']:
        try:
            if camera.is_frame_ready():
                camera.send_frame_over_eth(send_frame_data)
        except OSError as e:
            debug_print(f"Streaming loop error: {e}")
            state['streaming'] = False
        except KeyboardInterrupt:
            state['streaming'] = False
    
    debug_print(f"Stream ended after {state['frame_count']} frames")
    try:
        cl.close()
    except:
        pass
    
    # Recreate server socket
    s.close()
    time.sleep_ms(100)
    gc.collect()
    s = create_server_socket_fn()
    return s, "continue"


def _streamc_common(cl, s, create_server_socket_fn):
    """Common C streaming logic shared by all /streamc* endpoints.
    
    Caller must set the camera format BEFORE calling this.
    Manages the entire streaming lifecycle including socket recreation on exit.
    """
    # Gather optional sensor data for headers
    baro_header = b""
    if barometer.barometer_ready:
        try:
            temp_c, pressure_bar, _p_raw = barometer.read_barometer()
            if temp_c is not None and pressure_bar is not None:
                baro_header = f"X-barometer: {temp_c}C,{pressure_bar}bar".encode()
        except Exception:
            pass
    
    try:
        header = b"HTTP/1.1 200 OK\r\n"
        header += b"Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        header += b"Cache-Control: no-cache\r\n"
        header += b"Connection: keep-alive\r\n"
        if baro_header:
            header += baro_header + b"\r\n"
        header += b"\r\n"
        cl.send(header)
        debug_print("Stream client connected - header sent (C streaming)")
    except OSError as e:
        print(f"Failed to send header: {e}")
        cl.close()
        # Recreate socket and continue
        s.close()
        time.sleep_ms(100)
        gc.collect()
        s = create_server_socket_fn()
        return s, "continue"
    
    cl.settimeout(5.0)
    # Temperature reading setup
    temp_sensor = machine.ADC(4)
    VREF = 3.3
    BATCH_SIZE = 100
    debug_print("Starting C-based streaming loop")
    camera.stream_start()
    total_frames = 0
    sensor_task_idx = 0
    
    while True:
        sent = camera.stream_loop_c(cl, BATCH_SIZE)
        total_frames += sent
        if sent < BATCH_SIZE:
            break
        
        # Round-robin reading of sensors (1 per batch)
        if sensor_task_idx == 0:
            # Read MCU temperature between batches (~50us, invisible)
            raw = temp_sensor.read_u16()
            voltage = raw * VREF / 65535.0
            temp_c = 27.0 - ((voltage - 0.706) / 0.001721)
            camera.set_temperature(int(temp_c * 10))
        elif sensor_task_idx == 1:
            # External temperatures
            if tempsensor.adc_ready:
                ext = tempsensor.read_all_temperatures()
                def fmt(entry):
                    if entry is None:
                        return -999
                    t = entry.get("temp_c")
                    return t if t is not None else -999
                camera.set_ext_temperatures(
                    fmt(ext.get("headTemp1")),
                    fmt(ext.get("headTemp2")),
                    fmt(ext.get("headTemp3")),
                    fmt(ext.get("headTemp4")),
                    fmt(ext.get("headTemp5")),
                    fmt(ext.get("headTemp6"))
                )
            else:
                camera.set_ext_temperatures(-999, -999, -999, -999, -999, -999)
        elif sensor_task_idx == 2:
            # Read Barometer
            if barometer.barometer_ready:
                try:
                    baro_t, baro_p, _baro_raw = barometer.read_barometer()
                    b_t = int(baro_t) if baro_t is not None else -999
                    b_p = int(baro_p * 1000) if baro_p is not None else 0
                    camera.set_barometer(b_t, b_p)
                except Exception:
                    camera.set_barometer(-999, 0)
            else:
                camera.set_barometer(-999, 0)
        
        sensor_task_idx = (sensor_task_idx + 1) % 3
    
    debug_print(f"Stream ended after {total_frames} frames")
    try:
        cl.close()
    except:
        pass
    
    s.close()
    time.sleep_ms(100)
    gc.collect()
    s = create_server_socket_fn()
    return s, "continue"


def handle_streamc_vga(cl, s, create_server_socket_fn):
    """Handle /streamc_vga endpoint - VGA JPEG streaming"""
    ov5640_i2c.set_format("jpeg", "vga")
    return _streamc_common(cl, s, create_server_socket_fn)


def handle_streamc(cl, s, create_server_socket_fn):
    """Handle /streamc endpoint - alias for /streamc_vga"""
    return _streamc_common(cl, s, create_server_socket_fn)


def handle_streamc_rgb565(cl, s, create_server_socket_fn):
    """Handle /streamc_rgb565 endpoint - RGB565 240x320 streaming"""
    ov5640_i2c.set_format("rgb565")
    return _streamc_common(cl, s, create_server_socket_fn)


def handle_streamc_720p(cl, s, create_server_socket_fn):
    """Handle /streamc_720p endpoint - 720p JPEG streaming"""
    ov5640_i2c.set_format("jpeg", "720p")
    return _streamc_common(cl, s, create_server_socket_fn)


def handle_streamc_1080p(cl, s, create_server_socket_fn):
    """Handle /streamc_1080p endpoint - 1080p JPEG streaming"""
    ov5640_i2c.set_format("jpeg", "1080p")
    return _streamc_common(cl, s, create_server_socket_fn)


# ============== DISPATCHER ==============

def handle_getbarometer():
    """Handle /getbarometer endpoint - Get barometer temperature, pressure, and raw value"""
    if barometer.barometer_ready:
        baro_t, baro_p, baro_raw = barometer.read_barometer()
        body = json.dumps({"barometer_temp": baro_t, "pressure_bar": baro_p, "pressure_raw": baro_raw})
    else:
        body = json.dumps({"error": "Barometer not ready"})
    response = f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    return response


def handle_set_format(path):
    """Handle /set_format/{format}/{resolution} endpoint"""
    parts = path.split('/')
    if len(parts) >= 4:
        fmt = parts[2]
        res = parts[3]
        try:
            import ov5640_i2c
            ov5640_i2c.set_format(format=fmt, resolution=res)
            body = f"Switched to {fmt} {res}"
        except Exception as e:
            body = f"Error: {e}"
    else:
        body = "Invalid format request"
    
    response = f"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    return response


def handle_readi2cregister(path):
    """Handle /readi2cregister?address=0x... endpoint"""
    try:
        if '?' in path:
            query = path.split('?')[1]
            params = dict(p.split('=') for p in query.split('&') if '=' in p)
            addr_str = params.get('address', '0x00')
            address = int(addr_str, 16)
            
            import camera
            val = camera.read_register(address)
            body = f"0x{val:02x}"
        else:
            body = "Missing query parameters"
            return f"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    except Exception as e:
        body = f"Error: {e}"
        return f"HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
        
    response = f"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    return response


def handle_seti2cregister(path):
    """Handle /seti2cregister?address=0x...&value=0x... endpoint"""
    try:
        if '?' in path:
            query = path.split('?')[1]
            params = dict(p.split('=') for p in query.split('&') if '=' in p)
            
            addr_str = params.get('address', '0x00')
            val_str = params.get('value', '0x00')
            
            address = int(addr_str, 16)
            value = int(val_str, 16)
            
            import camera
            camera.write_register(address, value)
            body = f"Set {addr_str} to {val_str}"
        else:
            body = "Missing query parameters"
    except Exception as e:
        body = f"Error: {e}"
        
    response = f"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
    return response


def handle_request(path, cl, s, create_server_socket_fn, set_head_led_brightness_fn):
    """Main request dispatcher - routes to appropriate handler
    
    Args:
        path: The URL path from the HTTP request
        cl: Client socket
        s: Server socket
        create_server_socket_fn: Function to create a new server socket
        set_head_led_brightness_fn: Function to set LED brightness
    
    Returns:
        Tuple of (response_bytes, new_server_socket, should_continue)
        - response_bytes: The HTTP response to send (or None for streaming)
        - new_server_socket: The (possibly new) server socket
        - should_continue: True if caller should continue to next connection
    """
    if path == '/stream':
        new_s, _ = handle_stream(cl, s, create_server_socket_fn)
        return None, new_s, True
    elif path == '/streamc':
        new_s, _ = handle_streamc(cl, s, create_server_socket_fn)
        return None, new_s, True
    elif path == '/streamc_vga':
        new_s, _ = handle_streamc_vga(cl, s, create_server_socket_fn)
        return None, new_s, True
    elif path == '/streamc_rgb565':
        new_s, _ = handle_streamc_rgb565(cl, s, create_server_socket_fn)
        return None, new_s, True
    elif path == '/streamc_720p':
        new_s, _ = handle_streamc_720p(cl, s, create_server_socket_fn)
        return None, new_s, True
    elif path == '/streamc_1080p':
        new_s, _ = handle_streamc_1080p(cl, s, create_server_socket_fn)
        return None, new_s, True
    elif path == "/":
        return handle_root(), s, False
    elif path.startswith('/set_format/'):
        return handle_set_format(path), s, False
    elif path.startswith('/readi2cregister'):
        return handle_readi2cregister(path), s, False
    elif path.startswith('/seti2cregister'):
        return handle_seti2cregister(path), s, False
    elif path.startswith('/headled/'):
        return handle_headled(path, set_head_led_brightness_fn), s, False
    elif path == '/getmcutemperature':
        return handle_getmcutemperature(), s, False
    elif path == '/gettemperature/all':
        return handle_gettemperature_all(), s, False
    elif path.startswith('/gettemperature/'):
        return handle_gettemperature(path), s, False
    elif path == '/getbarometer':
        return handle_getbarometer(), s, False
    else:
        return handle_404(), s, False
