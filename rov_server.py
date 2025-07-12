import serial
from flask import Flask, request, jsonify

# Настройка последовательного порта
ser = serial.Serial('/dev/ttyAMA0', 115200, timeout=1)

app = Flask(__name__)

@app.route('/')
def index():
    return "Marine Dive ROV Server is running!"

@app.route('/control', methods=['POST'])
def control():
    data = request.get_json()

    if 'P1' in data:
        cmd = f"P1:{data['P1']}\n"
        ser.write(cmd.encode())
        print(f"Sent to Arduino: {cmd.strip()}")

    if 'P2' in data:
        cmd = f"P2:{data['P2']}\n"
        ser.write(cmd.encode())
        print(f"Sent to Arduino: {cmd.strip()}")

    if 'M' in data:
        cmd = f"M:{data['M']}\n"
        ser.write(cmd.encode())
        print(f"Sent to Arduino: {cmd.strip()}")

    if 'L' in data:
        light_value = 1 if data['L'] else 0
        cmd = f"L:{light_value}\n"
        ser.write(cmd.encode())
        print(f"Sent to Arduino: {cmd.strip()}")

    return jsonify({'status': 'OK'})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000, debug=True)
