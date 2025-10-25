# app.py — Marine Dive ROV Control via I2C (RPi master -> ESP32 slave @0x42)

import os, time, math, cv2
from collections import deque
from threading import Thread
from flask import Flask, request, jsonify, Response, render_template

from smbus2 import SMBus, i2c_msg

I2C_BUS_ID   = 1
ESP_I2C_ADDR = 0x42
I2C_RESP_N   = 32

LIGHT_INVERT = False

VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_FPS = 320, 240, 15

IMU_HZ = 50
CF_ALPHA = 0.98
DLPF_MODE = 6
GYRO_SENS = 131.0
ACCEL_SENS = 16384.0

FLASK_PORT = 8000

imu_data = {"roll":0.0,"pitch":0.0,"yaw":0.0,"ts":0.0}
imu_raw_buf = deque(maxlen=50)
imu_lines = 0

telemetry_data = {"depth":0.0,"temperature":20.5,"voltage":12.3,"current":1.2,"heading":0,"pressure":1013}
_last_light = {"on": False}

def i2c_send_raw(cmd: str) -> bool:
    try:
        payload = bytes(cmd + "\n", "ascii")
        with SMBus(I2C_BUS_ID) as bus:
            msg = i2c_msg.write(ESP_I2C_ADDR, payload)
            bus.i2c_rdwr(msg)
        return True
    except Exception as e:
        print("i2c_send_raw error:", e)
        return False

def i2c_read_status(n: int = I2C_RESP_N) -> str:
    try:
        with SMBus(I2C_BUS_ID) as bus:
            msg = i2c_msg.read(ESP_I2C_ADDR, n)
            bus.i2c_rdwr(msg)
            raw = bytes(msg)
        return raw.decode("utf-8", errors="ignore").strip("\x00\r\n ")
    except Exception as e:
        print("i2c_read_status error:", e)
        return ""

def esp_cmd_and_status(cmd: str) -> str:
    ok = i2c_send_raw(cmd)
    time.sleep(0.01)
    return i2c_read_status() if ok else ""

CAMERA_OK = False
picam2 = None
try:
    from picamera2 import Picamera2
    picam2 = Picamera2()
    cfg = picam2.create_video_configuration(
        main={"size": (VIDEO_WIDTH, VIDEO_HEIGHT), "format": "RGB888"}, buffer_count=3
    )
    picam2.configure(cfg); picam2.start(); time.sleep(0.3)
    CAMERA_OK = True
except Exception as e:
    print("❌ Camera init error:", e)

import smbus2
REG_PWR_MGMT_1, REG_SMPLRT_DIV, REG_CONFIG, REG_GYRO_CONFIG, REG_ACCEL_CONFIG = 0x6B,0x19,0x1A,0x1B,0x1C
REG_ACCEL_XOUT_H, REG_WHO_AM_I = 0x3B, 0x75
gx_bias=gy_bias=gz_bias=0.0
def _i2c_write(bus, addr, reg, val): bus.write_byte_data(addr, reg, val)
def _i2c_read_i16(bus, addr, reg):
    hi = bus.read_byte_data(addr, reg); lo = bus.read_byte_data(addr, reg+1)
    v = (hi<<8)|lo; return v-65536 if v&0x8000 else v
def _mpu_detect_addr(bus):
    for a in (0x68,0x69):
        try: bus.read_byte_data(a, REG_WHO_AM_I); return a
        except Exception: pass
    return None
def _mpu_init(bus, addr):
    _i2c_write(bus, addr, REG_PWR_MGMT_1, 0x00); time.sleep(0.05)
    _i2c_write(bus, addr, REG_CONFIG, DLPF_MODE & 7)
    _i2c_write(bus, addr, REG_GYRO_CONFIG, 0x00)
    _i2c_write(bus, addr, REG_ACCEL_CONFIG, 0x00)
    _i2c_write(bus, addr, REG_SMPLRT_DIV, 0x07); time.sleep(0.05)
def _mpu_read_all(bus, addr):
    ax=_i2c_read_i16(bus,addr,REG_ACCEL_XOUT_H+0)
    ay=_i2c_read_i16(bus,addr,REG_ACCEL_XOUT_H+2)
    az=_i2c_read_i16(bus,addr,REG_ACCEL_XOUT_H+4)
    gx=_i2c_read_i16(bus,addr,0x43); gy=_i2c_read_i16(bus,addr,0x45); gz=_i2c_read_i16(bus,addr,0x47)
    return ax,ay,az,gx,gy,gz
def _mpu_calibrate(bus, addr, samples=300):
    global gx_bias, gy_bias, gz_bias
    sx=sy=sz=0
    for _ in range(samples):
        _,_,_,gx,gy,gz=_mpu_read_all(bus,addr); sx+=gx; sy+=gy; sz+=gz; time.sleep(0.002)
    gx_bias,gy_bias,gz_bias = sx/samples, sy/samples, sz/samples
def imu_thread_fn():
    global imu_data, imu_lines
    try: bus = smbus2.SMBus(1)
    except Exception as e: print("❌ I2C bus error:", e); return
    TARGET_HZ = max(10, min(100, IMU_HZ)); period = 1.0/TARGET_HZ
    while True:
        try:
            addr=_mpu_detect_addr(bus)
            if addr is None: print("❌ MPU6050 not found. Retrying…"); time.sleep(3); continue
            _mpu_init(bus,addr); _mpu_calibrate(bus,addr,300)
            roll=pitch=yaw=0.0; next_t=time.monotonic()
            while True:
                t0=time.monotonic()
                ax,ay,az,gx,gy,gz=_mpu_read_all(bus,addr)
                ax_g=ax/ACCEL_SENS; ay_g=ay/ACCEL_SENS; az_g=az/ACCEL_SENS
                gx_dps=(gx-gx_bias)/GYRO_SENS; gy_dps=(gy-gy_bias)/GYRO_SENS; gz_dps=(gz-gz_bias)/GYRO_SENS
                dt=max(0.001, t0-(next_t-period))
                roll_acc=math.degrees(math.atan2(ay_g,az_g))
                pitch_acc=math.degrees(math.atan2(-ax_g,(ay_g*ay_g+az_g*az_g)**0.5))
                roll  = CF_ALPHA*(roll  + gx_dps*dt) + (1-CF_ALPHA)*roll_acc
                pitch = CF_ALPHA*(pitch + gy_dps*dt) + (1-CF_ALPHA)*pitch_acc
                yaw  += gz_dps*dt
                now=time.time()
                imu_data.update({"roll":roll,"pitch":pitch,"yaw":yaw,"ts":now})
                imu_raw_buf.append({"ax":ax,"ay":ay,"az":az,"gx":gx,"gy":gy,"gz":gz,"roll":roll,"pitch":pitch,"yaw":yaw,"ts":now})
                imu_lines+=1; next_t+=period; time.sleep(max(0.0,next_t-time.monotonic()))
        except Exception as e:
            print("IMU thread error:", e); time.sleep(3)

app = Flask(__name__, static_folder="static", template_folder="templates")

def generate_frames():
    if not CAMERA_OK or not picam2:
        while True:
            time.sleep(0.5)
            yield (b'--FRAME\r\nContent-Type: image/jpeg\r\n\r\n\r\n')
    else:
        while True:
            try:
                frame = picam2.capture_array()
                frame = cv2.rotate(frame, cv2.ROTATE_180)
                ok, buf = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 70])
                if not ok: time.sleep(0.05); continue
                yield (b'--FRAME\r\nContent-Type: image/jpeg\r\n\r\n' + buf.tobytes() + b'\r\n')
                time.sleep(1/max(1,VIDEO_FPS))
            except Exception:
                time.sleep(0.1)

@app.route('/')
def index(): return render_template('index.html')

@app.route('/video_feed')
def video_feed(): return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=FRAME')

@app.route('/snapshot')
def snapshot():
    if not CAMERA_OK or not picam2: return ("Camera not initialized", 503)
    try:
        frame = picam2.capture_array(); frame = cv2.rotate(frame, cv2.ROTATE_180)
        ok, buf = cv2.imencode('.jpg', frame, [85]);
        if not ok: return ("JPEG encode failed", 500)
        return Response(buf.tobytes(), mimetype="image/jpeg")
    except Exception as e:
        return (f"Snapshot error: {e}", 500)

def send_controls(payload):
    cmds=[]
    if 'P1' in payload: cmds.append(f"P1:{int(payload['P1'])}")
    if 'P2' in payload: cmds.append(f"P2:{int(payload['P2'])}")
    if 'P3' in payload: cmds.append(f"P3:{int(payload['P3'])}")
    if 'P4' in payload: cmds.append(f"P4:{int(payload['P4'])}")
    if 'M'  in payload: cmds.append(f"M:{int(payload['M'])}")
    if 'R'  in payload: cmds.append(f"R:{int(payload['R'])}")
    if 'L'  in payload:
        raw=1 if int(payload['L']) else 0
        val=(0 if raw==1 else 1) if LIGHT_INVERT else raw
        cmds.append(f"L:{val}")
        _last_light['on']=(raw==1)
    if not cmds: return False,"no commands",""
    status = esp_cmd_and_status("|".join(cmds))
    return True,"",status

@app.route('/control', methods=['POST'])
def control():
    try:
        data = request.get_json(force=True, silent=False) or {}
        ok, err, status = send_controls(data)
        return jsonify({'status':'OK' if ok else 'ERROR','sent':data,'esp_status':status,'error':err})
    except Exception as e:
        return jsonify({'status':'ERROR','message':str(e)}), 400

@app.route('/light_on')
def light_on():
    status = esp_cmd_and_status("L:1"); _last_light['on']=True
    return jsonify({"status":"OK","esp_status":status})

@app.route('/light_off')
def light_off():
    status = esp_cmd_and_status("L:0"); _last_light['on']=False
    return jsonify({"status":"OK","esp_status":status})

@app.route('/api/light/get')
def api_light_get(): return jsonify({"on": bool(_last_light.get("on", False))})

@app.route('/status')
def status(): return jsonify({'status':'running','esp_status': i2c_read_status(),'camera_ok':CAMERA_OK})

@app.route('/imu')
def imu(): return jsonify(imu_data)

@app.route('/imu_raw')
def imu_raw(): return jsonify({"count": imu_lines, "last": list(imu_raw_buf)})

@app.route('/telemetry')
def telemetry(): return jsonify(telemetry_data)

if __name__ == '__main__':
    if not os.path.exists('templates/index.html'):
        print("❌ templates/index.html not found!"); raise SystemExit(1)
    print("=== Marine Dive ROV (I2C master) ===")
    print(f"ESP I2C addr: 0x{ESP_I2C_ADDR:02X}  on bus {I2C_BUS_ID}")
    print(f"UI: http://0.0.0.0:{FLASK_PORT}")
    try:
        Thread(target=imu_thread_fn, daemon=True).start()
        app.run(host='0.0.0.0', port=FLASK_PORT, debug=False, threaded=True)
    finally:
        try:
            if picam2: picam2.stop()
        except Exception:
            pass
