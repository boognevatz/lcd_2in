"""Minimal camera streaming server on port 8081.
Only serves / (HTML viewer) and /stream (MJPEG-like raw RGB565 stream).
"""

import machine
import network
import socket
import time
import json
import gc
import camera


# ---------------------------------------------------------------------------
# Camera init (from lib/camera.py)
# ---------------------------------------------------------------------------
def init_camera(config):
    try:
        camera_config = config.get("camera")
        if camera_config is None:
            camera.init_cam()
        else:
            custom_regs_config = camera_config.get("custom_registers", [])
            custom_regs = []
            for reg in custom_regs_config:
                if not reg.get("enabled", True):
                    continue
                address_str = reg.get("address")
                value_str = reg.get("value")
                if address_str is None or value_str is None:
                    continue
                try:
                    custom_regs.append((int(address_str, 16), int(value_str, 16)))
                except ValueError:
                    continue

            if custom_regs:
                camera.init_cam_with_registers(custom_regs)
            else:
                camera.init_cam()

            camera.start_cam()

        print("Camera OK")
        return True
    except Exception as e:
        print(f"Camera FAILED: {e}")
        return False


# ---------------------------------------------------------------------------
# HTML page for / (stream viewer)
# ---------------------------------------------------------------------------
STREAM_PAGE = """<!DOCTYPE html><html><head><meta charset="UTF-8"><style>*{margin:0}canvas{display:block}</style></head><body><canvas id="c" width="240" height="320"></canvas><script>
const c=document.getElementById('c').getContext('2d'),W=240,H=320,FS=153616,T=8,HP=76800;
let mode=null;
async function s(){
const r=await fetch('/stream'),rd=r.body.getReader();
let sb=new Uint8Array(0);const bn=new TextEncoder().encode('--frame');
while(true){const{done,value}=await rd.read();if(done)break;
let cb=new Uint8Array(sb.length+value.length);cb.set(sb);cb.set(value,sb.length);sb=cb;
while(true){let bi=-1;for(let i=0;i<=sb.length-bn.length;i++){let m=true;for(let j=0;j<bn.length;j++)if(sb[i+j]!==bn[j]){m=false;break;}if(m){bi=i;break;}}
if(bi===-1)break;let ds=-1;for(let i=bi;i<sb.length-3;i++){if(sb[i]===13&&sb[i+1]===10&&sb[i+2]===13&&sb[i+3]===10){ds=i+4;break;}}
if(ds===-1||sb.length-ds<FS)break;d(sb.slice(ds,ds+FS));sb=sb.slice(ds+FS);}}}
function d(buf){const img=c.createImageData(W,H);
const pb=new Uint8Array(HP*2);pb.set(buf.subarray(T,T+HP));pb.set(buf.subarray(T+HP+T),HP);
if(mode===null&&pb.length>=8)mode=(pb[2]===0&&pb[3]===0&&pb[6]===0&&pb[7]===0)?'p':'k';
const tp=W*H;
if(mode==='p'){for(let i=0;i<tp;i+=2){const b=i*2;if(b+1>=pb.length)break;const v=(pb[b+1]<<8)|pb[b];const r=(v>>11)&0x1F,g=(v>>5)&0x3F,bl=v&0x1F;const R=(r<<3)|(r>>2),G=(g<<2)|(g>>4),B=(bl<<3)|(bl>>2);let x=i*4;img.data[x]=R;img.data[x+1]=G;img.data[x+2]=B;img.data[x+3]=255;img.data[x+4]=R;img.data[x+5]=G;img.data[x+6]=B;img.data[x+7]=255;}}
else{for(let i=0;i<tp;i+=2){const b=i*2;if(b+3>=pb.length)break;let v=(pb[b+3]<<8)|pb[b+2],r=(v>>11)&0x1F,g=(v>>5)&0x3F,bl=v&0x1F,x=i*4;img.data[x]=(r<<3)|(r>>2);img.data[x+1]=(g<<2)|(g>>4);img.data[x+2]=(bl<<3)|(bl>>2);img.data[x+3]=255;v=(pb[b+1]<<8)|pb[b];r=(v>>11)&0x1F;g=(v>>5)&0x3F;bl=v&0x1F;x=(i+1)*4;img.data[x]=(r<<3)|(r>>2);img.data[x+1]=(g<<2)|(g>>4);img.data[x+2]=(bl<<3)|(bl>>2);img.data[x+3]=255;}}
c.putImageData(img,0,0);}s();
</script></body></html>"""


# ---------------------------------------------------------------------------
# Socket helpers
# ---------------------------------------------------------------------------
def create_server_socket(port):
    gc.collect()
    s = socket.socket()
    try:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    except OSError:
        pass
    s.bind(socket.getaddrinfo('0.0.0.0', port)[0][-1])
    s.listen(5)
    s.setblocking(False)
    return s


def _recreate_socket(old_socket, port):
    try:
        old_socket.close()
    except:
        pass
    time.sleep_ms(50)
    return create_server_socket(port)


# ---------------------------------------------------------------------------
# Web server (port 8081 only)
# ---------------------------------------------------------------------------
def start_webserver():
    s_camera = create_server_socket(8081)
    print("Server ready | stream :8081")

    stream_cl = None
    stream_first_frame = True
    stream_frame_count = 0

    while True:
        try:
            # --- Stream: send one frame if client active ---
            if stream_cl is not None:
                result = camera.send_frame_data_c(stream_cl, stream_first_frame)
                if result is True:
                    stream_first_frame = False
                    stream_frame_count += 1
                elif result is False:
                    print(f"[8081] Stream ended after {stream_frame_count} frames")
                    try:
                        stream_cl.close()
                    except:
                        pass
                    stream_cl = None
                    stream_first_frame = True
                    stream_frame_count = 0
                    s_camera = create_server_socket(8081)

            # --- Accept new connections (skip during streaming) ---
            if stream_cl is None:
                try:
                    cl, addr = s_camera.accept()
                    print(f"[8081] Client connected from {addr}")
                    cl.settimeout(5.0)

                    request = cl.recv(1024).decode('utf-8')
                    request_line = request.split('\r\n')[0]
                    path = request_line.split(' ')[1] if len(request_line.split(' ')) > 1 else '/'
                    print(f"[8081] {request_line} -> {path}")

                    if path == '/stream':
                        header = b"HTTP/1.1 200 OK\r\n"
                        header += b"Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                        header += b"Cache-Control: no-cache\r\n"
                        header += b"Connection: keep-alive\r\n"
                        header += b"\r\n"
                        cl.send(header)

                        stream_cl = cl
                        stream_first_frame = True
                        stream_frame_count = 0
                        print("[8081] Stream client registered")

                    elif path == '/' or path == '':
                        data = ("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" + STREAM_PAGE).encode()
                        cl.send(data)
                        cl.close()
                        s_camera = _recreate_socket(s_camera, 8081)

                    else:
                        cl.send(b"HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nNot Found")
                        cl.close()
                        s_camera = _recreate_socket(s_camera, 8081)

                except OSError:
                    pass

            time.sleep_ms(1)

        except Exception as e:
            print(f"Server error: {e}")
            import sys
            sys.print_exception(e)
            if stream_cl is not None:
                try:
                    stream_cl.close()
                except:
                    pass
                stream_cl = None
                stream_first_frame = True
                stream_frame_count = 0
            try:
                s_camera.close()
            except:
                pass
            time.sleep_ms(500)
            gc.collect()
            s_camera = create_server_socket(8081)


# ---------------------------------------------------------------------------
# Hardware init & main
# ---------------------------------------------------------------------------

# Load config (only camera section matters)
try:
    with open('config.json', 'r') as f:
        config = json.load(f)
except Exception as e:
    print(f"Warning: config.json not found ({e}), using defaults")
    config = {}

# W5500 Ethernet
cs = machine.Pin(17, machine.Pin.OUT)
rst = machine.Pin(20, machine.Pin.OUT)

cs.value(1)
time.sleep_ms(10)

rst.value(0)
time.sleep_ms(100)
rst.value(1)
time.sleep_ms(500)

spi = machine.SPI(0,
                  baudrate=40000000,
                  polarity=0,
                  phase=0,
                  sck=machine.Pin(18),
                  mosi=machine.Pin(19),
                  miso=machine.Pin(16))
time.sleep_ms(100)

nic = network.WIZNET5K(spi, cs, rst)
nic.active(True)
time.sleep_ms(100)
nic.ifconfig(('172.16.1.1', '255.255.255.0', '172.16.1.1', '8.8.8.8'))
time.sleep_ms(500)

if nic.active():
    print(f"Ethernet OK: {nic.ifconfig()[0]}")
else:
    print("ERROR: Ethernet failed")

# Camera
init_camera(config)

# Run
if nic.active():
    try:
        start_webserver()
    except KeyboardInterrupt:
        print("\nShutdown")
    except Exception as e:
        print(f"\nERROR: Server: {e}")
        import sys
        sys.print_exception(e)
else:
    print("ERROR: No network")
